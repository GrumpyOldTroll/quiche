// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/structured_headers.h"

#include <cmath>
#include <string>
#include <utility>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "common/platform/api/quiche_logging.h"

namespace quiche {
namespace structured_headers {

namespace {

#define DIGIT "0123456789"
#define LCALPHA "abcdefghijklmnopqrstuvwxyz"
#define UCALPHA "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define TCHAR DIGIT LCALPHA UCALPHA "!#$%&'*+-.^_`|~"
// https://tools.ietf.org/html/draft-ietf-httpbis-header-structure-09#section-3.9
constexpr char kTokenChars09[] = DIGIT UCALPHA LCALPHA "_-.:%*/";
// https://www.rfc-editor.org/rfc/rfc8941.html#section-3.3.4
constexpr char kTokenChars[] = TCHAR ":/";
// https://tools.ietf.org/html/draft-ietf-httpbis-header-structure-09#section-3.1
constexpr char kKeyChars09[] = DIGIT LCALPHA "_-";
// https://www.rfc-editor.org/rfc/rfc8941.html#section-3.1.2
constexpr char kKeyChars[] = DIGIT LCALPHA "_-.*";
constexpr char kSP[] = " ";
constexpr char kOWS[] = " \t";
#undef DIGIT
#undef LCALPHA
#undef UCALPHA

// https://www.rfc-editor.org/rfc/rfc8941.html#section-3.3.1
constexpr int64_t kMaxInteger = 999'999'999'999'999L;
constexpr int64_t kMinInteger = -999'999'999'999'999L;

// Smallest value which is too large for an sh-decimal. This is the smallest
// double which will round up to 1e12 when serialized, which exceeds the range
// for sh-decimal. Any float less than this should round down. This behaviour is
// verified by unit tests.
constexpr double kTooLargeDecimal = 1e12 - 0.0005;

// Removes characters in remove from the beginning of s.
void StripLeft(absl::string_view& s, absl::string_view remove) {
  size_t i = 0;
  while (i < s.size() && memchr(remove.data(), s[i], remove.size())) {
    ++i;
  }
  if (i > 0) s.remove_prefix(i);
}

// Parser for (a subset of) Structured Headers for HTTP defined in [SH09] and
// [RFC8941]. [SH09] compatibility is retained for use by Web Packaging, and can
// be removed once that spec is updated, and users have migrated to new headers.
// [SH09] https://tools.ietf.org/html/draft-ietf-httpbis-header-structure-09
// [RFC8941] https://www.rfc-editor.org/rfc/rfc8941.html
class StructuredHeaderParser {
 public:
  enum DraftVersion {
    kDraft09,
    kFinal,
  };
  explicit StructuredHeaderParser(absl::string_view str, DraftVersion version)
      : input_(str), version_(version) {
    // [SH09] 4.2 Step 1.
    // Discard any leading OWS from input_string.
    // [RFC8941] 4.2 Step 2.
    // Discard any leading SP characters from input_string.
    SkipWhitespaces();
  }
  StructuredHeaderParser(const StructuredHeaderParser&) = delete;
  StructuredHeaderParser& operator=(const StructuredHeaderParser&) = delete;

  // Callers should call this after ReadSomething(), to check if parser has
  // consumed all the input successfully.
  bool FinishParsing() {
    // [SH09] 4.2 Step 7.
    // Discard any leading OWS from input_string.
    // [RFC8941] 4.2 Step 6.
    // Discard any leading SP characters from input_string.
    SkipWhitespaces();
    // [SH09] 4.2 Step 8. [RFC8941] 4.2 Step 7.
    // If input_string is not empty, fail parsing.
    return input_.empty();
  }

  // Parses a List of Lists ([SH09] 4.2.4).
  absl::optional<ListOfLists> ReadListOfLists() {
    QUICHE_CHECK_EQ(version_, kDraft09);
    ListOfLists result;
    while (true) {
      std::vector<Item> inner_list;
      while (true) {
        absl::optional<Item> item(ReadBareItem());
        if (!item) return absl::nullopt;
        inner_list.push_back(std::move(*item));
        SkipWhitespaces();
        if (!ConsumeChar(';')) break;
        SkipWhitespaces();
      }
      result.push_back(std::move(inner_list));
      SkipWhitespaces();
      if (!ConsumeChar(',')) break;
      SkipWhitespaces();
    }
    return result;
  }

  // Parses a List ([RFC8941] 4.2.1).
  absl::optional<List> ReadList() {
    QUICHE_CHECK_EQ(version_, kFinal);
    List members;
    while (!input_.empty()) {
      absl::optional<ParameterizedMember> member(ReadItemOrInnerList());
      if (!member) return absl::nullopt;
      members.push_back(std::move(*member));
      SkipOWS();
      if (input_.empty()) break;
      if (!ConsumeChar(',')) return absl::nullopt;
      SkipOWS();
      if (input_.empty()) return absl::nullopt;
    }
    return members;
  }

  // Parses an Item ([RFC8941] 4.2.3).
  absl::optional<ParameterizedItem> ReadItem() {
    absl::optional<Item> item = ReadBareItem();
    if (!item) return absl::nullopt;
    absl::optional<Parameters> parameters = ReadParameters();
    if (!parameters) return absl::nullopt;
    return ParameterizedItem(std::move(*item), std::move(*parameters));
  }

  // Parses a bare Item ([RFC8941] 4.2.3.1, though this is also the algorithm
  // for parsing an Item from [SH09] 4.2.7).
  absl::optional<Item> ReadBareItem() {
    if (input_.empty()) {
      DVLOG(1) << "ReadBareItem: unexpected EOF";
      return absl::nullopt;
    }
    switch (input_.front()) {
      case '"':
        return ReadString();
      case '*':
        if (version_ == kDraft09) return ReadByteSequence();
        return ReadToken();
      case ':':
        if (version_ == kFinal) return ReadByteSequence();
        return absl::nullopt;
      case '?':
        return ReadBoolean();
      default:
        if (input_.front() == '-' || absl::ascii_isdigit(input_.front()))
          return ReadNumber();
        if (absl::ascii_isalpha(input_.front())) return ReadToken();
        return absl::nullopt;
    }
  }

  // Parses a Dictionary ([RFC8941] 4.2.2).
  absl::optional<Dictionary> ReadDictionary() {
    QUICHE_CHECK_EQ(version_, kFinal);
    Dictionary members;
    while (!input_.empty()) {
      absl::optional<std::string> key(ReadKey());
      if (!key) return absl::nullopt;
      absl::optional<ParameterizedMember> member;
      if (ConsumeChar('=')) {
        member = ReadItemOrInnerList();
        if (!member) return absl::nullopt;
      } else {
        absl::optional<Parameters> parameters;
        parameters = ReadParameters();
        if (!parameters) return absl::nullopt;
        member = ParameterizedMember{Item(true), std::move(*parameters)};
      }
      members[*key] = std::move(*member);
      SkipOWS();
      if (input_.empty()) break;
      if (!ConsumeChar(',')) return absl::nullopt;
      SkipOWS();
      if (input_.empty()) return absl::nullopt;
    }
    return members;
  }

  // Parses a Parameterised List ([SH09] 4.2.5).
  absl::optional<ParameterisedList> ReadParameterisedList() {
    QUICHE_CHECK_EQ(version_, kDraft09);
    ParameterisedList items;
    while (true) {
      absl::optional<ParameterisedIdentifier> item =
          ReadParameterisedIdentifier();
      if (!item) return absl::nullopt;
      items.push_back(std::move(*item));
      SkipWhitespaces();
      if (!ConsumeChar(',')) return items;
      SkipWhitespaces();
    }
  }

 private:
  // Parses a Parameterised Identifier ([SH09] 4.2.6).
  absl::optional<ParameterisedIdentifier> ReadParameterisedIdentifier() {
    QUICHE_CHECK_EQ(version_, kDraft09);
    absl::optional<Item> primary_identifier = ReadToken();
    if (!primary_identifier) return absl::nullopt;

    ParameterisedIdentifier::Parameters parameters;

    SkipWhitespaces();
    while (ConsumeChar(';')) {
      SkipWhitespaces();

      absl::optional<std::string> name = ReadKey();
      if (!name) return absl::nullopt;

      Item value;
      if (ConsumeChar('=')) {
        auto item = ReadBareItem();
        if (!item) return absl::nullopt;
        value = std::move(*item);
      }
      if (!parameters.emplace(*name, value).second) {
        DVLOG(1) << "ReadParameterisedIdentifier: duplicated parameter: "
                 << *name;
        return absl::nullopt;
      }
      SkipWhitespaces();
    }
    return ParameterisedIdentifier(std::move(*primary_identifier),
                                   std::move(parameters));
  }

  // Parses an Item or Inner List ([RFC8941] 4.2.1.1).
  absl::optional<ParameterizedMember> ReadItemOrInnerList() {
    QUICHE_CHECK_EQ(version_, kFinal);
    std::vector<Item> member;
    bool member_is_inner_list = (!input_.empty() && input_.front() == '(');
    if (member_is_inner_list) {
      return ReadInnerList();
    } else {
      auto item = ReadItem();
      if (!item) return absl::nullopt;
      return ParameterizedMember(std::move(item->item),
                                 std::move(item->params));
    }
  }

  // Parses Parameters ([RFC8941] 4.2.3.2)
  absl::optional<Parameters> ReadParameters() {
    Parameters parameters;
    absl::flat_hash_set<std::string> keys;

    while (ConsumeChar(';')) {
      SkipWhitespaces();

      absl::optional<std::string> name = ReadKey();
      if (!name) return absl::nullopt;
      bool is_duplicate_key = !keys.insert(*name).second;

      Item value{true};
      if (ConsumeChar('=')) {
        auto item = ReadBareItem();
        if (!item) return absl::nullopt;
        value = std::move(*item);
      }
      if (is_duplicate_key) {
        for (auto& param : parameters) {
          if (param.first == name) {
            param.second = std::move(value);
            break;
          }
        }
      } else {
        parameters.emplace_back(std::move(*name), std::move(value));
      }
    }
    return parameters;
  }

  // Parses an Inner List ([RFC8941] 4.2.1.2).
  absl::optional<ParameterizedMember> ReadInnerList() {
    QUICHE_CHECK_EQ(version_, kFinal);
    if (!ConsumeChar('(')) return absl::nullopt;
    std::vector<ParameterizedItem> inner_list;
    while (true) {
      SkipWhitespaces();
      if (ConsumeChar(')')) {
        absl::optional<Parameters> parameters;
        parameters = ReadParameters();
        if (!parameters) return absl::nullopt;
        return ParameterizedMember(std::move(inner_list), true,
                                   std::move(*parameters));
      }
      auto item = ReadItem();
      if (!item) return absl::nullopt;
      inner_list.push_back(std::move(*item));
      if (input_.empty() || (input_.front() != ' ' && input_.front() != ')'))
        return absl::nullopt;
    }
    QUICHE_NOTREACHED();
    return absl::nullopt;
  }

  // Parses a Key ([SH09] 4.2.2, [RFC8941] 4.2.3.3).
  absl::optional<std::string> ReadKey() {
    if (version_ == kDraft09) {
      if (input_.empty() || !absl::ascii_islower(input_.front())) {
        LogParseError("ReadKey", "lcalpha");
        return absl::nullopt;
      }
    } else {
      if (input_.empty() ||
          (!absl::ascii_islower(input_.front()) && input_.front() != '*')) {
        LogParseError("ReadKey", "lcalpha | *");
        return absl::nullopt;
      }
    }
    const char* allowed_chars =
        (version_ == kDraft09 ? kKeyChars09 : kKeyChars);
    size_t len = input_.find_first_not_of(allowed_chars);
    if (len == absl::string_view::npos) len = input_.size();
    std::string key(input_.substr(0, len));
    input_.remove_prefix(len);
    return key;
  }

  // Parses a Token ([SH09] 4.2.10, [RFC8941] 4.2.6).
  absl::optional<Item> ReadToken() {
    if (input_.empty() ||
        !(absl::ascii_isalpha(input_.front()) || input_.front() == '*')) {
      LogParseError("ReadToken", "ALPHA");
      return absl::nullopt;
    }
    size_t len = input_.find_first_not_of(version_ == kDraft09 ? kTokenChars09
                                                               : kTokenChars);
    if (len == absl::string_view::npos) len = input_.size();
    std::string token(input_.substr(0, len));
    input_.remove_prefix(len);
    return Item(std::move(token), Item::kTokenType);
  }

  // Parses a Number ([SH09] 4.2.8, [RFC8941] 4.2.4).
  absl::optional<Item> ReadNumber() {
    bool is_negative = ConsumeChar('-');
    bool is_decimal = false;
    size_t decimal_position = 0;
    size_t i = 0;
    for (; i < input_.size(); ++i) {
      if (i > 0 && input_[i] == '.' && !is_decimal) {
        is_decimal = true;
        decimal_position = i;
        continue;
      }
      if (!absl::ascii_isdigit(input_[i])) break;
    }
    if (i == 0) {
      LogParseError("ReadNumber", "DIGIT");
      return absl::nullopt;
    }
    if (!is_decimal) {
      // [RFC8941] restricts the range of integers further.
      if (version_ == kFinal && i > 15) {
        LogParseError("ReadNumber", "integer too long");
        return absl::nullopt;
      }
    } else {
      if (version_ != kFinal && i > 16) {
        LogParseError("ReadNumber", "float too long");
        return absl::nullopt;
      }
      if (version_ == kFinal && decimal_position > 12) {
        LogParseError("ReadNumber", "decimal too long");
        return absl::nullopt;
      }
      if (i - decimal_position > (version_ == kFinal ? 4 : 7)) {
        LogParseError("ReadNumber", "too many digits after decimal");
        return absl::nullopt;
      }
      if (i == decimal_position) {
        LogParseError("ReadNumber", "no digits after decimal");
        return absl::nullopt;
      }
    }
    std::string output_number_string(input_.substr(0, i));
    input_.remove_prefix(i);

    if (is_decimal) {
      // Convert to a 64-bit double, and return if the conversion is
      // successful.
      double f;
      if (!absl::SimpleAtod(output_number_string, &f)) return absl::nullopt;
      return Item(is_negative ? -f : f);
    } else {
      // Convert to a 64-bit signed integer, and return if the conversion is
      // successful.
      int64_t n;
      if (!absl::SimpleAtoi(output_number_string, &n)) return absl::nullopt;
      QUICHE_CHECK(version_ != kFinal ||
                   (n <= kMaxInteger && n >= kMinInteger));
      return Item(is_negative ? -n : n);
    }
  }

  // Parses a String ([SH09] 4.2.9, [RFC8941] 4.2.5).
  absl::optional<Item> ReadString() {
    std::string s;
    if (!ConsumeChar('"')) {
      LogParseError("ReadString", "'\"'");
      return absl::nullopt;
    }
    while (!ConsumeChar('"')) {
      size_t i = 0;
      for (; i < input_.size(); ++i) {
        if (!absl::ascii_isprint(input_[i])) {
          DVLOG(1) << "ReadString: non printable-ASCII character";
          return absl::nullopt;
        }
        if (input_[i] == '"' || input_[i] == '\\') break;
      }
      if (i == input_.size()) {
        DVLOG(1) << "ReadString: missing closing '\"'";
        return absl::nullopt;
      }
      s.append(std::string(input_.substr(0, i)));
      input_.remove_prefix(i);
      if (ConsumeChar('\\')) {
        if (input_.empty()) {
          DVLOG(1) << "ReadString: backslash at string end";
          return absl::nullopt;
        }
        if (input_[0] != '"' && input_[0] != '\\') {
          DVLOG(1) << "ReadString: invalid escape";
          return absl::nullopt;
        }
        s.push_back(input_.front());
        input_.remove_prefix(1);
      }
    }
    return s;
  }

  // Parses a Byte Sequence ([SH09] 4.2.11, [RFC8941] 4.2.7).
  absl::optional<Item> ReadByteSequence() {
    char delimiter = (version_ == kDraft09 ? '*' : ':');
    if (!ConsumeChar(delimiter)) {
      LogParseError("ReadByteSequence", "delimiter");
      return absl::nullopt;
    }
    size_t len = input_.find(delimiter);
    if (len == absl::string_view::npos) {
      DVLOG(1) << "ReadByteSequence: missing closing delimiter";
      return absl::nullopt;
    }
    std::string base64(input_.substr(0, len));
    // Append the necessary padding characters.
    base64.resize((base64.size() + 3) / 4 * 4, '=');

    std::string binary;
    if (!absl::Base64Unescape(base64, &binary)) {
      DVLOG(1) << "ReadByteSequence: failed to decode base64: " << base64;
      return absl::nullopt;
    }
    input_.remove_prefix(len);
    ConsumeChar(delimiter);
    return Item(std::move(binary), Item::kByteSequenceType);
  }

  // Parses a Boolean ([RFC8941] 4.2.8).
  // Note that this only parses ?0 and ?1 forms from SH version 10+, not the
  // previous ?F and ?T, which were not needed by any consumers of SH version 9.
  absl::optional<Item> ReadBoolean() {
    if (!ConsumeChar('?')) {
      LogParseError("ReadBoolean", "'?'");
      return absl::nullopt;
    }
    if (ConsumeChar('1')) {
      return Item(true);
    }
    if (ConsumeChar('0')) {
      return Item(false);
    }
    return absl::nullopt;
  }

  // There are several points in the specs where the handling of whitespace
  // differs between Draft 9 and the final RFC. In those cases, Draft 9 allows
  // any OWS character, while the RFC allows only a U+0020 SPACE.
  void SkipWhitespaces() {
    if (version_ == kDraft09) {
      StripLeft(input_, kOWS);
    } else {
      StripLeft(input_, kSP);
    }
  }

  void SkipOWS() { StripLeft(input_, kOWS); }

  bool ConsumeChar(char expected) {
    if (!input_.empty() && input_.front() == expected) {
      input_.remove_prefix(1);
      return true;
    }
    return false;
  }

  void LogParseError(const char* func, const char* expected) {
    DVLOG(1) << func << ": " << expected << " expected, got "
             << (input_.empty() ? "EOS"
                                : "'" + std::string(input_.substr(0, 1)) + "'");
  }

  absl::string_view input_;
  DraftVersion version_;
};

// Serializer for (a subset of) Structured Field Values for HTTP defined in
// [RFC8941]. Note that this serializer does not attempt to support [SH09].
class StructuredHeaderSerializer {
 public:
  StructuredHeaderSerializer() = default;
  ~StructuredHeaderSerializer() = default;
  StructuredHeaderSerializer(const StructuredHeaderSerializer&) = delete;
  StructuredHeaderSerializer& operator=(const StructuredHeaderSerializer&) =
      delete;

  std::string Output() { return output_.str(); }

  // Serializes a List ([RFC8941] 4.1.1).
  bool WriteList(const List& value) {
    bool first = true;
    for (const auto& member : value) {
      if (!first) output_ << ", ";
      if (!WriteParameterizedMember(member)) return false;
      first = false;
    }
    return true;
  }

  // Serializes an Item ([RFC8941] 4.1.3).
  bool WriteItem(const ParameterizedItem& value) {
    if (!WriteBareItem(value.item)) return false;
    return WriteParameters(value.params);
  }

  // Serializes an Item ([RFC8941] 4.1.3).
  bool WriteBareItem(const Item& value) {
    if (value.is_string()) {
      // Serializes a String ([RFC8941] 4.1.6).
      output_ << "\"";
      for (const char& c : value.GetString()) {
        if (!absl::ascii_isprint(c)) return false;
        if (c == '\\' || c == '\"') output_ << "\\";
        output_ << c;
      }
      output_ << "\"";
      return true;
    }
    if (value.is_token()) {
      // Serializes a Token ([RFC8941] 4.1.7).
      if (value.GetString().empty() ||
          !(absl::ascii_isalpha(value.GetString().front()) ||
            value.GetString().front() == '*'))
        return false;
      if (value.GetString().find_first_not_of(kTokenChars) != std::string::npos)
        return false;
      output_ << value.GetString();
      return true;
    }
    if (value.is_byte_sequence()) {
      // Serializes a Byte Sequence ([RFC8941] 4.1.8).
      output_ << ":";
      output_ << absl::Base64Escape(value.GetString());
      output_ << ":";
      return true;
    }
    if (value.is_integer()) {
      // Serializes an Integer ([RFC8941] 4.1.4).
      if (value.GetInteger() > kMaxInteger || value.GetInteger() < kMinInteger)
        return false;
      output_ << value.GetInteger();
      return true;
    }
    if (value.is_decimal()) {
      // Serializes a Decimal ([RFC8941] 4.1.5).
      double decimal_value = value.GetDecimal();
      if (!std::isfinite(decimal_value) ||
          fabs(decimal_value) >= kTooLargeDecimal)
        return false;

      // Handle sign separately to simplify the rest of the formatting.
      if (decimal_value < 0) output_ << "-";
      // Unconditionally take absolute value to ensure that -0 is serialized as
      // "0.0", with no negative sign, as required by spec. (4.1.5, step 2).
      decimal_value = fabs(decimal_value);
      double remainder = fmod(decimal_value, 0.002);
      if (remainder == 0.0005) {
        // Value ended in exactly 0.0005, 0.0025, 0.0045, etc. Round down.
        decimal_value -= 0.0005;
      } else if (remainder == 0.0015) {
        // Value ended in exactly 0.0015, 0.0035, 0,0055, etc. Round up.
        decimal_value += 0.0005;
      } else {
        // Standard rounding will work in all other cases.
        decimal_value = round(decimal_value * 1000.0) / 1000.0;
      }

      // Use standard library functions to write the decimal, and then truncate
      // if necessary to conform to spec.

      // Maximum is 12 integer digits, one decimal point, three fractional
      // digits, and a null terminator.
      char buffer[17];
      absl::SNPrintF(buffer, std::size(buffer), "%#.3f", decimal_value);

      // Strip any trailing 0s after the decimal point, but leave at least one
      // digit after it in all cases. (So 1.230 becomes 1.23, but 1.000 becomes
      // 1.0.)
      absl::string_view formatted_number(buffer);
      auto truncate_index = formatted_number.find_last_not_of('0');
      if (formatted_number[truncate_index] == '.') truncate_index++;
      output_ << formatted_number.substr(0, truncate_index + 1);
      return true;
    }
    if (value.is_boolean()) {
      // Serializes a Boolean ([RFC8941] 4.1.9).
      output_ << (value.GetBoolean() ? "?1" : "?0");
      return true;
    }
    return false;
  }

  // Serializes a Dictionary ([RFC8941] 4.1.2).
  bool WriteDictionary(const Dictionary& value) {
    bool first = true;
    for (const auto& [dict_key, dict_value] : value) {
      if (!first) output_ << ", ";
      if (!WriteKey(dict_key)) return false;
      first = false;
      if (!dict_value.member_is_inner_list && !dict_value.member.empty() &&
          dict_value.member.front().item.is_boolean() &&
          dict_value.member.front().item.GetBoolean()) {
        if (!WriteParameters(dict_value.params)) return false;
      } else {
        output_ << "=";
        if (!WriteParameterizedMember(dict_value)) return false;
      }
    }
    return true;
  }

 private:
  bool WriteParameterizedMember(const ParameterizedMember& value) {
    // Serializes a parameterized member ([RFC8941] 4.1.1).
    if (value.member_is_inner_list) {
      if (!WriteInnerList(value.member)) return false;
    } else {
      QUICHE_CHECK_EQ(value.member.size(), 1UL);
      if (!WriteItem(value.member[0])) return false;
    }
    return WriteParameters(value.params);
  }

  bool WriteInnerList(const std::vector<ParameterizedItem>& value) {
    // Serializes an inner list ([RFC8941] 4.1.1.1).
    output_ << "(";
    bool first = true;
    for (const ParameterizedItem& member : value) {
      if (!first) output_ << " ";
      if (!WriteItem(member)) return false;
      first = false;
    }
    output_ << ")";
    return true;
  }

  bool WriteParameters(const Parameters& value) {
    // Serializes a parameter list ([RFC8941] 4.1.1.2).
    for (const auto& param_name_and_value : value) {
      const std::string& param_name = param_name_and_value.first;
      const Item& param_value = param_name_and_value.second;
      output_ << ";";
      if (!WriteKey(param_name)) return false;
      if (!param_value.is_null()) {
        if (param_value.is_boolean() && param_value.GetBoolean()) continue;
        output_ << "=";
        if (!WriteBareItem(param_value)) return false;
      }
    }
    return true;
  }

  bool WriteKey(const std::string& value) {
    // Serializes a Key ([RFC8941] 4.1.1.3).
    if (value.empty()) return false;
    if (value.find_first_not_of(kKeyChars) != std::string::npos) return false;
    if (!absl::ascii_islower(value[0]) && value[0] != '*') return false;
    output_ << value;
    return true;
  }

  std::ostringstream output_;
};

}  // namespace

Item::Item() {}
Item::Item(const std::string& value, Item::ItemType type)
    : type_(type), string_value_(value) {}
Item::Item(std::string&& value, Item::ItemType type)
    : type_(type), string_value_(std::move(value)) {
  QUICHE_CHECK(type_ == kStringType || type_ == kTokenType ||
               type_ == kByteSequenceType);
}
Item::Item(const char* value, Item::ItemType type)
    : Item(std::string(value), type) {}
Item::Item(int64_t value) : type_(kIntegerType), integer_value_(value) {}
Item::Item(double value) : type_(kDecimalType), decimal_value_(value) {}
Item::Item(bool value) : type_(kBooleanType), boolean_value_(value) {}

bool operator==(const Item& lhs, const Item& rhs) {
  if (lhs.type_ != rhs.type_) return false;
  switch (lhs.type_) {
    case Item::kNullType:
      return true;
    case Item::kStringType:
    case Item::kTokenType:
    case Item::kByteSequenceType:
      return lhs.string_value_ == rhs.string_value_;
    case Item::kIntegerType:
      return lhs.integer_value_ == rhs.integer_value_;
    case Item::kDecimalType:
      return lhs.decimal_value_ == rhs.decimal_value_;
    case Item::kBooleanType:
      return lhs.boolean_value_ == rhs.boolean_value_;
  }
  QUICHE_NOTREACHED();
  return false;
}

ParameterizedItem::ParameterizedItem(const ParameterizedItem&) = default;
ParameterizedItem& ParameterizedItem::operator=(const ParameterizedItem&) =
    default;
ParameterizedItem::ParameterizedItem(Item id, const Parameters& ps)
    : item(std::move(id)), params(ps) {}
ParameterizedItem::~ParameterizedItem() = default;

ParameterizedMember::ParameterizedMember() = default;
ParameterizedMember::ParameterizedMember(const ParameterizedMember&) = default;
ParameterizedMember& ParameterizedMember::operator=(
    const ParameterizedMember&) = default;
ParameterizedMember::ParameterizedMember(std::vector<ParameterizedItem> id,
                                         bool member_is_inner_list,
                                         const Parameters& ps)
    : member(std::move(id)),
      member_is_inner_list(member_is_inner_list),
      params(ps) {}
ParameterizedMember::ParameterizedMember(std::vector<ParameterizedItem> id,
                                         const Parameters& ps)
    : member(std::move(id)), member_is_inner_list(true), params(ps) {}
ParameterizedMember::ParameterizedMember(Item id, const Parameters& ps)
    : member({{std::move(id), {}}}), member_is_inner_list(false), params(ps) {}
ParameterizedMember::~ParameterizedMember() = default;

ParameterisedIdentifier::ParameterisedIdentifier(
    const ParameterisedIdentifier&) = default;
ParameterisedIdentifier& ParameterisedIdentifier::operator=(
    const ParameterisedIdentifier&) = default;
ParameterisedIdentifier::ParameterisedIdentifier(Item id, const Parameters& ps)
    : identifier(std::move(id)), params(ps) {}
ParameterisedIdentifier::~ParameterisedIdentifier() = default;

Dictionary::Dictionary() = default;
Dictionary::Dictionary(const Dictionary&) = default;
Dictionary::Dictionary(std::vector<DictionaryMember> members)
    : members_(std::move(members)) {}
Dictionary::~Dictionary() = default;
std::vector<DictionaryMember>::iterator Dictionary::begin() {
  return members_.begin();
}
std::vector<DictionaryMember>::const_iterator Dictionary::begin() const {
  return members_.begin();
}
std::vector<DictionaryMember>::iterator Dictionary::end() {
  return members_.end();
}
std::vector<DictionaryMember>::const_iterator Dictionary::end() const {
  return members_.end();
}
ParameterizedMember& Dictionary::operator[](std::size_t idx) {
  return members_[idx].second;
}
const ParameterizedMember& Dictionary::operator[](std::size_t idx) const {
  return members_[idx].second;
}
ParameterizedMember& Dictionary::at(std::size_t idx) { return (*this)[idx]; }
const ParameterizedMember& Dictionary::at(std::size_t idx) const {
  return (*this)[idx];
}
ParameterizedMember& Dictionary::operator[](absl::string_view key) {
  auto it = absl::c_find_if(
      members_, [key](const auto& member) { return member.first == key; });
  if (it != members_.end()) return it->second;
  members_.push_back({std::string(key), ParameterizedMember()});
  return members_.back().second;
}
ParameterizedMember& Dictionary::at(absl::string_view key) {
  auto it = absl::c_find_if(
      members_, [key](const auto& member) { return member.first == key; });
  QUICHE_CHECK(it != members_.end()) << "Provided key not found in dictionary";
  return it->second;
}
const ParameterizedMember& Dictionary::at(absl::string_view key) const {
  auto it = absl::c_find_if(
      members_, [key](const auto& member) { return member.first == key; });
  QUICHE_CHECK(it != members_.end()) << "Provided key not found in dictionary";
  return it->second;
}
bool Dictionary::empty() const { return members_.empty(); }
std::size_t Dictionary::size() const { return members_.size(); }
bool Dictionary::contains(absl::string_view key) const {
  for (auto& member : members_) {
    if (member.first == key) return true;
  }
  return false;
}

absl::optional<ParameterizedItem> ParseItem(absl::string_view str) {
  StructuredHeaderParser parser(str, StructuredHeaderParser::kFinal);
  absl::optional<ParameterizedItem> item = parser.ReadItem();
  if (item && parser.FinishParsing()) return item;
  return absl::nullopt;
}

absl::optional<Item> ParseBareItem(absl::string_view str) {
  StructuredHeaderParser parser(str, StructuredHeaderParser::kFinal);
  absl::optional<Item> item = parser.ReadBareItem();
  if (item && parser.FinishParsing()) return item;
  return absl::nullopt;
}

absl::optional<ParameterisedList> ParseParameterisedList(
    absl::string_view str) {
  StructuredHeaderParser parser(str, StructuredHeaderParser::kDraft09);
  absl::optional<ParameterisedList> param_list = parser.ReadParameterisedList();
  if (param_list && parser.FinishParsing()) return param_list;
  return absl::nullopt;
}

absl::optional<ListOfLists> ParseListOfLists(absl::string_view str) {
  StructuredHeaderParser parser(str, StructuredHeaderParser::kDraft09);
  absl::optional<ListOfLists> list_of_lists = parser.ReadListOfLists();
  if (list_of_lists && parser.FinishParsing()) return list_of_lists;
  return absl::nullopt;
}

absl::optional<List> ParseList(absl::string_view str) {
  StructuredHeaderParser parser(str, StructuredHeaderParser::kFinal);
  absl::optional<List> list = parser.ReadList();
  if (list && parser.FinishParsing()) return list;
  return absl::nullopt;
}

absl::optional<Dictionary> ParseDictionary(const absl::string_view& str) {
  StructuredHeaderParser parser(str, StructuredHeaderParser::kFinal);
  absl::optional<Dictionary> dictionary = parser.ReadDictionary();
  if (dictionary && parser.FinishParsing()) return dictionary;
  return absl::nullopt;
}

absl::optional<std::string> SerializeItem(const Item& value) {
  StructuredHeaderSerializer s;
  if (s.WriteItem(ParameterizedItem(value, {}))) return s.Output();
  return absl::nullopt;
}

absl::optional<std::string> SerializeItem(const ParameterizedItem& value) {
  StructuredHeaderSerializer s;
  if (s.WriteItem(value)) return s.Output();
  return absl::nullopt;
}

absl::optional<std::string> SerializeList(const List& value) {
  StructuredHeaderSerializer s;
  if (s.WriteList(value)) return s.Output();
  return absl::nullopt;
}

absl::optional<std::string> SerializeDictionary(const Dictionary& value) {
  StructuredHeaderSerializer s;
  if (s.WriteDictionary(value)) return s.Output();
  return absl::nullopt;
}

}  // namespace structured_headers
}  // namespace quiche
