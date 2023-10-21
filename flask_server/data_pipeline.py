import json
import zmq
from flask import Flask, render_template
from flask_sse import sse
import multiprocessing

app = Flask(__name__)
app.config["REDIS_URL"] = "redis://localhost"  # You may need to configure this accordingly
app.register_blueprint(sse, url_prefix='/stream')

# Define the buffer size
buffer_size = 10

# Create a manager to share data between processes
manager = multiprocessing.Manager()
event_buffer = manager.list()

# Define the circular buffer and x-axis values
x_values = list(range(1, buffer_size + 1))

@app.route('/')
def index():
    return render_template('index.html')

def send_event_data(event_data):
    # Extract the hodo_y_data from event data
    hodo_y_data = event_data.get("traces_hodo_y", [])[2]

    # Create a dictionary containing the specific data
    specific_data = {
        "event_number": event_data.get("event", "Event Information"),
        "hodo_y_data": hodo_y_data,
    }

    # Convert the ListProxy to a regular list
    event_list = list(event_buffer)

    if len(event_list) >= buffer_size:
        event_list.pop(0)
    event_list.append(specific_data)
    print(event_list)
    with app.app_context():
        # Publish the regular list
        sse.publish({"data": event_list[0]}, type='event_update')


def start_event_processing():
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    socket.connect("tcp://127.0.0.1:5555")  # Replace with your ZeroMQ endpoint

    # Subscribe to all messages
    socket.setsockopt_string(zmq.SUBSCRIBE, "")

    while True:
        event_data = json.loads(socket.recv_string())
        print("Received event data:", event_data)
        send_event_data(event_data)

if __name__ == '__main__':
    # Start the Flask server in a separate process
    flask_process = multiprocessing.Process(target=app.run, kwargs={"debug": True})
    flask_process.start()

    # Start the event processing loop
    start_event_processing()
