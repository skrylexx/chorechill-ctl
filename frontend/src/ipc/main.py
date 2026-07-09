import socket

class IPCClient:
    def __init__(self, path="/tmp/chorechill-ctl.sock"):
        self.path = path

    def send_command(self, message):
        # open socket, send command, receive response and close
        try:
            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            # 1 second timeout to avoid freezing the UI
            s.settimeout(1.0)
            
            s.connect(self.path)
            s.sendall(message.encode('utf-8'))
            
            response = s.recv(1024)
            s.close()
            
            return response.decode('utf-8')
            
        except Exception as e:
            return f"ERR: {e}"