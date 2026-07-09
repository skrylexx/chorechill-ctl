import socket

SOCKET_PATH = "/tmp/fan_control.sock"

class IPCClient:
    def __init__(self, path=SOCKET_PATH):
        self.path = path

    def send_command(self, message):
        """send_command sends a command to the C daemon via Unix socket and returns the response."""
        try:
            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        
            # don't block indefinitely; set a timeout for the socket operations
            s.settimeout(1.0) 
            s.connect(self.path)

            # send instruction to the C daemon
            s.sendall(message.encode('utf-8'))

            # receive response from the C daemon
            response = s.recv(1024)
            s.close()
            
            return response.decode('utf-8')

        # handle exceptions and return error messages
        except FileNotFoundError:
            return "ERR: BACKEND_OFFLINE"
        except socket.timeout:
            return "ERR: TIMEOUT"
        except Exception as e:
            return f"ERR: {e}"


if __name__ == "__main__":
    client = IPCClient()
    print("Tentative de communication avec le C...")
    
    reponse = client.send_command("GET_DATA")
    print(f"Réponse reçue : {reponse}")