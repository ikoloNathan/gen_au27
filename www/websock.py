'''
Created on 12 Oct 2025

@author: Nathan Ikolo
'''

# server.py
import asyncio
from pathlib import Path
from aiohttp import web

ROOT = Path(__file__).parent
INDEX =  "./index.html"

connected = set()  # track active websockets for broadcast, if you want

async def index(request: web.Request) -> web.StreamResponse:
    # Serve your uploaded index.html
    return web.FileResponse(INDEX)

async def ws_handler(request: web.Request) -> web.StreamResponse:
    ws = web.WebSocketResponse(heartbeat=5)  # send ping every 5s
    await ws.prepare(request)

    connected.add(ws)
    try:
        async for msg in ws:
            if msg.type == web.WSMsgType.TEXT:
                # Echo back; also broadcast to others (optional)
                print(f"{msg.data}")
                await ws.send_str(f"{msg.data}")
                # Broadcast to everyone else:
                for peer in list(connected):
                    if peer is not ws:
                        await peer.send_str(f"{msg.data}")
            elif msg.type == web.WSMsgType.BINARY:
                await ws.send_bytes(msg.data)  # or ignore
            elif msg.type == web.WSMsgType.ERROR:
                # Log ws.exception() if desired
                pass
    finally:
        connected.discard(ws)

    return ws  # closed

def create_app() -> web.Application:
    app = web.Application()
    app.router.add_get("/", index)
    app.router.add_get("/ws", ws_handler)

    # Optional: serve any additional static files in the same folder
    # e.g., /app.js, /styles.css
    app.router.add_static("/", str(ROOT), show_index=False)
    return app

if __name__ == "__main__":
    web.run_app(create_app(), host="127.0.0.1", port=8000)
