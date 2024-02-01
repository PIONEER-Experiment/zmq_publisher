import zmq
import json
import os
from channel_info import ChannelInfo
from multiprocessing import Lock

def read_config(file_name="config.json"):
    script_dir = os.path.dirname(os.path.abspath(__file__))
    parent_dir = os.path.dirname(script_dir)
    config_file_path = os.path.join(parent_dir, file_name)

    try:
        with open(config_file_path, "r") as config_file:
            config = json.load(config_file)
        return config
    except FileNotFoundError:
        print(f"Config file '{config_file_path}' not found.")
    except json.JSONDecodeError as e:
        print(f"Error reading config file: {e}")

    return None

def generate_address_to_channels(config):
    address_to_channels = {}

    if config is not None and "data-channels" in config:
        channels = config["data-channels"]

        for channel_name, channel_config in channels.items():
            zmq_address = channel_config.get("zmq-address", "")
            channel_id = channel_config.get("name", "")  # Get the "name" field or an empty string if missing

            if zmq_address:
                if zmq_address not in address_to_channels:
                    address_to_channels[zmq_address] = []

                address_to_channels[zmq_address].append(channel_id)  # Append the channel_id to the list

    return address_to_channels

def get_unique_address_channel_pairs(address_to_channels):
    unique_address_channel_pairs = []

    for address, channels in address_to_channels.items():
        for channel in channels:
            unique_address_channel_pairs.append((address, channel))

    return unique_address_channel_pairs

def create_channel_info_mapping(address_to_channels):
    unique_pairs = get_unique_address_channel_pairs(address_to_channels)

    unique_pairs_to_channel_info = {}

    for address, channel_name in unique_pairs:
        print(address, channel_name)
        channel_info = ChannelInfo(address,channel_name)
        unique_pairs_to_channel_info[(address, channel_name)] = channel_info

    return unique_pairs_to_channel_info

def listen_to_channels(shared_data):
    context = zmq.Context()
    sockets = {}  # Dictionary to store the sockets

    for address, channels in shared_data['address_to_channels'].items():
        socket = context.socket(zmq.SUB)
        socket.connect(address)

        for channel_name in channels:
            socket.setsockopt_string(zmq.SUBSCRIBE, channel_name)

        sockets[address] = socket

    while True:
        for address, socket in sockets.items():
            try:
                parts = socket.recv_multipart(zmq.DONTWAIT)  # Receive a multipart message without blocking

                if len(parts) >= 2:
                    channel_name = parts[0].decode()  # Assuming the first part is the channel name
                    content = parts[1]  # The rest of the parts can be considered as content

                    data_size = sum(len(part) for part in parts)  # Calculate the data size

                    # Update the ChannelInfo using the extracted channel_name
                    shared_data['channel_info_mapping'][(address, channel_name)].update_publish(data_size,content)
                    #print(shared_data['channel_info_mapping'][(address, channel_name)])
            except zmq.Again:
                pass
