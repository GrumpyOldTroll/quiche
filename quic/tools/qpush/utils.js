let unit ;

function unitlib()
{
    if (unit)
        return unit;

    unit = require('unitmath')

    unit = unit.config({
        definitions: {
        units: {
            o: {
                value: '1 B',
                prefixes: 'SHORT'
            }
        }
        }
    })

    return unit;
}

function parseUnit(str, unit)
{
    try {
        return unitlib()(str).to(unit).value;
    }
    catch(e) {
        program.error('error: ' +e.message);
    }
}

function parseRate(str) { return parseUnit(str, 'b/s'); }
function parseTime(str) { return parseUnit(str, 'ms'); }
function parseBits(str) { return parseUnit(str, 'b'); }

module.exports = {
    parseBits, parseRate, parseTime, parseUnit
}