from flask import Flask, render_template, jsonify
from flask_socketio import SocketIO, emit
from data_collection import listen_to_channels, read_config, generate_address_to_channels, create_channel_info_mapping
import threading

app = Flask(__name__, static_url_path='/static')
app.config['SECRET_KEY'] = 'secret!'
socketio = SocketIO(app)

def background_thread(shared_data):
    listen_to_channels(shared_data)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/data')
def get_data():
    # Return the channel information as JSON with keys converted to strings
    json_data = {str(key): value.to_dict() for key, value in shared_data['channel_info_mapping'].items()}
    return jsonify(json_data)


if __name__ == '__main__':
    # Read config
    config = read_config("config.json")

    if not config:
        print("Error reading config.")
        exit()

    # Generate address to channels mapping
    address_to_channels = generate_address_to_channels(config)

    # Create channel info mapping
    channel_info_mapping = create_channel_info_mapping(address_to_channels)

    # Create shared data dictionary
    shared_data = {
        'address_to_channels': address_to_channels,
        'channel_info_mapping': channel_info_mapping,
    }

    # Start the background thread
    bg_thread = threading.Thread(target=background_thread, args=(shared_data,))
    bg_thread.start()

    # Start the Flask app with SocketIO support
    socketio.run(app, host='0.0.0.0', port=8000, debug=True)
