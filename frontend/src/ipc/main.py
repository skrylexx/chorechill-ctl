import socket

SOCKET_PATH = "/tmp/chorechill-ctl.sock"

class IPCClient:
    def __init__(self, path=SOCKET_PATH):
        self.path = path

    def send_command(self, message):
        # open socket, send command, receive response and close
        try:
            s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            # 1 second timeout to avoid freezing the UI
            s.settimeout(1.0)

            s.connect(self.path)

            # send instruction to the C daemon
            s.sendall(message.encode('utf-8'))

            # receive response from the C daemon
            response = s.recv(1024)
            s.close()

            return response.decode('utf-8')

        # handle exceptions and return normalised error strings
        except FileNotFoundError:
            return "ERR: BACKEND_OFFLINE"
        except socket.timeout:
            return "ERR: TIMEOUT"
        except Exception as e:
            return f"ERR: {e}"