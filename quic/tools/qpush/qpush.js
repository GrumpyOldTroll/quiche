const net = require('net');
const path = require('path');

const { parseRate, parseTime } = require('./utils')

const { Command, Argument } = require('commander');
const program = new Command();
/*
var client = net.createConnection("/tmp/test.sock");

const protobuf = require('google-protobuf')


const { PushServer, PushServerActivatePool } = require('./quic/core/proto/push_server_pb')

const cmd = new PushServer()

cmd.setActivatePool(new PushServerActivatePool())
cmd.getActivatePool().setPoolId(42);

const buffer = cmd.serializeBinary();

console.log(buffer, buffer.byteLength)

client.on("connect", function () {
    console.log('Connect')
    console.log('sending message: activate_pool 42')

    const writer = new protobuf.BinaryWriter()

    writer.writeUint32(1, buffer.byteLength);

    const size_buffer = writer.getResultBuffer().slice(1);

    client.write(size_buffer); //write size of the message

    client.write(buffer);

    client.write(size_buffer); //write size of the message

    client.write(buffer);

    console.log('write', size_buffer)
    console.log('write', buffer)

    console.log('write', size_buffer)
    console.log('write', buffer)
});

client.on("data", function (data) {
    console.log('data', data)
});

client.on('error', function () {
    console.log('Error')
})*/

function activatePool(id) {
    console.log(id)
}

function deactivatePool(id) {

}

function makePool(type) {

}


function makeChannel(id, src, group, port, options) {
    console.log(options)
}

function makeUnicast(type) {
    
}

function finishChannel(pool_id, chan_id) {
    
}

function rmChannel(id, options)
{

}

function setChannel(id, options)
{
    
}
function getChannel(id, options)
{
    
}

function datagramsChannel(id)
{
    
}

function removeStream(id)
{
    
}

function receiveStream(id)
{
    
}

Command.prototype.snippet = function (f){ return f(this); }

const pool = program.command('pool')
    .description('Manage pools');

pool.command('make')
    .addArgument(new Argument('<type>', 'Pool type').choices(['alternatives','ordered-layers','arbitrary-layers']))
    .description('Make new pool')
    .action(makePool);

pool.command('activate')
    .argument('<id>', 'Pool id', parseInt)
    .description('Activate pool')
    .action(activatePool)

pool.command('deactivate')
    .argument('<id>', 'Pool id', parseInt)
    .description('Deactivate pool')
    .action(deactivatePool)
    ;

const chan = program.command('channel')
    .description('Manage channels');

function channelProperties(cmd)
{
    return cmd
    .option('--max-rate <rate>', 'Max debit rate', parseRate)
    .option('--max-idle <time>', 'Max IDLE time', parseTime)
    .option('--allow-datagrams', 'Allow datagrams', false)
    .option('--key', 'Key')
    .option('--key-delay', 'Key delay', 0)
    .option('--key-algorithm', 'Key algorithm', 0);
}

chan.command('make')
    .description('Make new channel')
    .argument('<id>', 'Pool id', parseInt)
    .argument('<source>', 'Source IPv4/IPv6')
    .argument('<group>', 'Group IPv4/IPv6')
    .argument('<port>', 'Port')
    .snippet(channelProperties)
    .action(makeChannel);

    chan.command('rm')
    .argument('<id>', 'Channel id', parseInt)
    .description('Remove channel')
    .action(rmChannel);

    chan.command('set')
    .argument('<id>', 'Channel id', parseInt)
    .description('Set channel properties')
    .snippet(channelProperties)
    .action(setChannel);

    chan.command('get')
    .argument('<id>', 'Channel id', parseInt)
    .description('Get channel properties')
    .snippet(channelProperties)
    .action(getChannel);

    chan.command('datagrams')
    .argument('<id>', 'Channel id', parseInt)
    .description('Receive datagrams from the channel to the standard output')
    .snippet(channelProperties)
    .action(datagramsChannel);


const stream = program.command('stream')
    .description('Manage streams');

    stream.command('receive')
    .argument('<id>', 'Stream id', parseInt)
    .description('Receive in-order data in stream frames to the standard output')
    .action(receiveStream)

    stream.command('rm')
    .description('Finish the stream and remove it')
    .argument('<id>', 'Stream id', parseInt)
    .action(removeStream)

program.parse();