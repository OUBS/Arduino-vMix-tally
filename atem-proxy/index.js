const { Atem } = require('atem-connection');
const process = require('process');
const net = require('net');

const TALLY_PORT = 8099;
const ATEM_INPUT_MAP = {
    1: 1,
    2: 2,
    3: 3,
    4: 4,
    5: 5,
    6: 6,
    7: 7,
    8: 8,
    3010: 9,
    3011: 10,
    3020: 11,
    3021: 12,
    2001: 13,
    2002: 14,
    0: 15,
    1000: 16,
};

if (process.argv.length != 3) {
    console.error('Wrong number of arguments. Usage:');
    console.error('    ' + process.argv[1] + ' atem-ip-address');
    process.exit(1);
}

class TallyServer {
    constructor() {
        this.sockets = [];
        this.state = ['0'];   // When we haven't got any states from the atem, start with all inactive
        net.createServer(socket => {
            this.sockets.push(socket);
            socket.write('VERSION OK 23.0.0.1\r\n');
            // Send the initial state. We might not be allowed to do this in actual protocol, but our tally receivers
            // accept this.
            socket.write('TALLY OK ' + this.state.join('') + '\r\n');
            socket.on('close', () => {
                this.sockets = this.sockets.filter(t => t !== socket);
            });
            socket.on('error', () => {
                console.log('Error');
                this.sockets = this.sockets.filter(t => t !== socket);
            });
            socket.on('data', data => {
                // Ignore data from the tally
            });
        }).listen(TALLY_PORT);
    }

    setProgramPreview(program, preview, aux) {
        const input_count = Object.keys(ATEM_INPUT_MAP).length;
        const state = Array(2 * input_count).fill('0');
        // Fill in preview first and then program to overwrite that
        for (let p of preview) {
            if (p in ATEM_INPUT_MAP) {
                state[ATEM_INPUT_MAP[p] - 1] = '2';
            }
        }
        for (let p of program) {
            if (p in ATEM_INPUT_MAP) {
                state[ATEM_INPUT_MAP[p] - 1] = '1';
            }
        }
        // Set aux input
        for (let p of aux) {
            if (p in ATEM_INPUT_MAP) {
                state[input_count + ATEM_INPUT_MAP[p] - 1] = '1';
            }
        }
        // Send the state to all listeners
        for (let socket of this.sockets) {
            socket.write('TALLY OK ' + state.join('') + '\r\n');
        }
        this.state = state;
    }
}

const address = process.argv[2];
const tally_server = new TallyServer();

function connect() {
    const atem = new Atem();

    atem.on('error', err => {
        console.error('ERR:  ' + err);
    });
    atem.on('info', info => {
        console.info('INFO: ' + info);
    });

    atem.connect(address);

    function update_state(state) {
        for (let mixEffect of state.video.mixEffects) {
            if (mixEffect.transitionPosition.inTransition) {
                // While effect is in transit, show both inputs as active
                tally_server.setProgramPreview([mixEffect.previewInput, mixEffect.programInput], [], state.video.auxilliaries);
            } else {
                tally_server.setProgramPreview([mixEffect.programInput], [mixEffect.previewInput], state.video.auxilliaries);
            }
        }
    }

    atem.on('connected', () => {
        update_state(atem.state);
        atem.on('stateChanged', (state, msg) => {
            update_state(state);
        });
    });
}

connect();