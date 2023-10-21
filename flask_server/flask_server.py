from flask import Flask, render_template
from flask_sse import sse

app = Flask(__name__)
app.register_blueprint(sse, url_prefix='/stream')

# Define the circular buffer and x-axis values
buffer_size = 10
event_buffer = []
x_values = list(range(1, buffer_size + 1))

@app.route('/')
def index():
    # Render an HTML template that initializes the SSE connection
    return render_template('index.html')

def start_flask_server(event_buffer):
    # Inside the Flask server, update the circular buffer and push updates using SSE
    while True:
        if len(event_buffer) > buffer_size:
            event_buffer.pop(0)  # Remove the oldest data point
        sse.publish({"data": event_buffer}, type='event_update')
        time.sleep(1)  # Adjust the sleep time as needed to control update frequency

if __name__ == '__main__':
    # Start Flask server in a separate process
    flask_process = multiprocessing.Process(target=start_flask_server, args=(event_buffer,))
    flask_process.start()
    app.run(debug=True)