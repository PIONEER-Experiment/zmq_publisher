// data_utils.js

/**
 * Retrieve the last object from a specific channel's data.
 * @param {object} data - The data object containing channel data.
 * @param {string} channelName - The name of the channel.
 * @returns {object|null} The last object from the channel's data, or null if not found.
 */
function getLastObjectByChannel(data, channelName) {
    for (var channel in data) {
        var channelData = data[channel];
        if (channelData.channel_name === channelName) {
            return getLastObject(channelData);
        }
    }
    console.error("Channel '" + channelName + "' not found.");
    return null;
}

/**
 * Retrieve the last object from channel data.
 * @param {object} channelData - The channel data object.
 * @returns {object|null} The last object from the channel's data, or null if the data is invalid.
 */
function getLastObject(channelData) {
    // Access the "latest_content" string
    var latestContentString = String(channelData.latest_content);

    try {
        // Parse the modified string as a JSON array
        var wrappedContentString = '[' + latestContentString + ']';
        var parsedContentArray = JSON.parse(wrappedContentString);

        // Extract the last element from the array
        var lastObject = parsedContentArray[parsedContentArray.length - 1];

        return lastObject;
    } catch (error) {
        // Handle parsing errors if needed
        console.error("Error parsing JSON:", error);
        return null; // or handle the error in an appropriate way
    }
}

// Function to check if two objects are equal
function isDataEqual(obj1, obj2) {
    return JSON.stringify(obj1) === JSON.stringify(obj2);
}
