import zmq
import json
import os
import threading
import multiprocessing
from channel_info import ChannelInfo
from flask import Flask, request, jsonify
from data_collection import read_config, generate_address_to_channels, get_unique_address_channel_pairs, create_channel_info_mapping, listen_to_channels

# Create a manager to hold shared data
manager = multiprocessing.Manager()
shared_data = manager.dict()

app = Flask(__name__)

def start_web_server():
    app.run(host='0.0.0.0', port=8000)  # Run the Flask app on port 8000

@app.route('/')
def hello():
    return "Hello, Flask!"

@app.route('/restart', methods=['POST'])
def restart():
    # Perform any actions needed for a restart
    print("Received restart request")
    return 'Restarting...'

@app.route('/data', methods=['GET'])
def get_data():
    try:
        # Retrieve data from the data collection thread
        data = get_data_from_data_collection_thread()
        return jsonify(data)
    except Exception as e:
        app.logger.error(f"An error occurred: {str(e)}")
        return 'Error occurred', 500


def get_data_from_data_collection_thread():
    data = []
    for key, channel_info in shared_data['channel_info_mapping'].items():
        data.append(str(channel_info))
    return data


def main():
    config = read_config("config.json")

    if config:
        address_to_channels = generate_address_to_channels(config)
        channel_info_mapping = create_channel_info_mapping(address_to_channels)
        #shared_data = {
        #    'address_to_channels': address_to_channels,
        #    'channel_info_mapping': channel_info_mapping
        #}
        shared_data['address_to_channels'] = address_to_channels
        shared_data['channel_info_mapping'] = channel_info_mapping
        data_collection_thread = threading.Thread(target=listen_to_channels, args=(shared_data,))
        data_collection_thread.daemon = True
        data_collection_thread.start()

        web_server_thread = threading.Thread(target=start_web_server)
        web_server_thread.start()

if __name__ == '__main__':
    main()
