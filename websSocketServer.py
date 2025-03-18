import asyncio
import websockets
import cv2
import numpy as np

connected_clients = set()
angles = {'link1_y': 0, 'link2_x': 90, 'link3_x': 0}
update = False
q2 = 0

def calcIK(x, y, z):
    global q2
    
    xp = np.sqrt(x*x + z*z)
    q1 = np.arctan2(-z , x)
  
        
    c2 = ((x*x+y*y+z*z) - 1.8**2 - 1.7**2) / (2 * 1.8 * 1.7)
    if abs(c2) > 1: return None, None, None
    if c2 == 1: return q1, np.arctan2(y, xp), 0
    if c2 == -1: return q1, np.arctan2(y, xp), np.pi
    
    q3 = -np.arccos(c2)
    theta = np.arctan2(y, xp)
    
    q21 = theta - np.arctan2(1.7 * np.sin(q3), 1.8 + 1.7 * np.cos(q3))
    q22 = theta - np.arctan2(1.7 * np.sin(-q3), 1.8 + 1.7 * np.cos(-q3))
    
    if (abs(q21 - q2) > abs(q22 - q2)):
        q2 = q22
    else:
        q2 = q21   
         
    return q1, q2, -q3

def on_trackbar(val, axis):
    global angles
    global update
    
    update = True
    angles[axis] = val

async def echo(websocket):
    global update
    print("Client connected")
    connected_clients.add(websocket)
    update = True
    try:
        async for message in websocket:
            msg = message.split(",")
            q1, q2, q3 = calcIK(float(msg[0]), float(msg[1]), float(msg[2]))
            if not q1: continue
            await websocket.send(f"{q1 - np.pi / 2},{q2},{q3}")
    except websockets.exceptions.ConnectionClosed as e:
        print(f"Client disconnected: {e}")
    finally:
        connected_clients.remove(websocket)

async def send_periodic_messages():
    global update
    while True:
        cv2.waitKey(1)
        
        if update:
            update = False
            if connected_clients:
                    for client in connected_clients.copy():
                        try:
                            msg = f"{angles['link1_y']*(3.1415926535 / 180)},{angles['link2_x']*(3.1415926535 / 180) + 3.1415926535 },{angles['link3_x']*(3.1415926535 / 180) - 3.1415926535 / 2}"
                            await client.send(msg)
                            
                        except websockets.exceptions.ConnectionClosed: 
                            connected_clients.remove(client)
                        
        await asyncio.sleep(0.1)  # Send every second
        
cv2.namedWindow("Angle Control")
cv2.createTrackbar("link1", "Angle Control", 0, 360, lambda v: on_trackbar(v, 'link1_y'))
cv2.createTrackbar("link2", "Angle Control", 90, 180, lambda v: on_trackbar(v, 'link2_x'))
cv2.createTrackbar("link3", "Angle Control", 0, 180, lambda v: on_trackbar(v, 'link3_x'))

async def main():
    print("hello")
    server = await websockets.serve(echo, "localhost", 8765)
    print("WebSocket server started on ws://localhost:8765")
    await asyncio.gather(server.wait_closed(), send_periodic_messages())
    
asyncio.run(main())