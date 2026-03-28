/**
 * RME MCP Stdio-to-HTTP Bridge
 *
 * Standard MCP clients (like Claude Desktop) communicate via stdin/stdout.
 * This script runs as the MCP Server process. It reads stdin line-by-line,
 * forwards the JSON-RPC to the RME HTTP MCP backend (localhost:8080),
 * and prints the HTTP response back to stdout.
 */

const http = require('http');
const readline = require('readline');

const PORT = 8080;
const HOST = '127.0.0.1';

const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
    terminal: false
});

rl.on('line', (line) => {
    if (!line.trim()) return;

    let reqId = null;
    try {
        const parsed = JSON.parse(line);
        reqId = parsed.id || null;
    } catch (e) {
        console.log(JSON.stringify({
            jsonrpc: "2.0",
            error: { code: -32700, message: "Parse error" },
            id: null
        }));
        return;
    }

    const options = {
        hostname: HOST,
        port: PORT,
        path: '/',
        method: 'POST',
        timeout: 10000,
        headers: {
            'Content-Type': 'application/json',
            'Content-Length': Buffer.byteLength(line)
        }
    };

    const req = http.request(options, (res) => {
        let responseData = '';

        res.on('data', (chunk) => {
            responseData += chunk;
        });

        res.on('end', () => {
            if (responseData.trim()) {
                console.log(responseData.trim());
            }
        });
    });

    req.on('timeout', () => {
        req.destroy();
        console.log(JSON.stringify({
            jsonrpc: "2.0",
            error: { code: -32001, message: "Request timeout" },
            id: reqId
        }));
    });

    req.on('error', (e) => {
        console.log(JSON.stringify({
            jsonrpc: "2.0",
            error: { code: -32002, message: "Connection error: " + e.message },
            id: reqId
        }));
    });

    req.write(line);
    req.end();
});

// Keep process alive
process.stdin.resume();
