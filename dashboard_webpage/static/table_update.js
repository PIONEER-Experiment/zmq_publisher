// table_update.js

var socket = io.connect('http://' + document.domain + ':' + location.port);

// List of keys to display in the table and their corresponding labels
var keysToDisplay = [
  'address', 'channel_name', 'total_publishes', 'total_data_size', 'average_data_size',
  'rate_publishes', 'rate_data', 'start_time', 'last_receive_time'
];
var labels = [
  'Address', 'Channel Name', 'Total Publishes', 'Total Data Size', 'Average Data Size',
  'Publish Rate', 'Data Rate', 'Start Time', 'Last Receive Time'
];

// Function to update the table on the webpage
function updateTable(data) {
  var tableBody = document.getElementById('data-container');
  tableBody.innerHTML = '';  // Clear existing rows

  // Loop through the data and create a new row for each channel
  for (var channel in data) {
    var rowData = data[channel];
    var row = tableBody.insertRow();

    // Populate the row cells with channel information
    for (var i = 0; i < keysToDisplay.length; i++) {
      var key = keysToDisplay[i];
      var cell = row.insertCell();
      var value = (key === 'start_time' || key === 'last_receive_time') ? new Date(rowData[key] * 1000).toLocaleString() : rowData[key];
      cell.innerHTML = value;
    }
  }
}

// Initial data update
fetch('/data')
  .then(response => response.json())
  .then(data => updateTable(data));

// SocketIO event handler for data updates
socket.on('update_data', updateTable);

// Periodically fetch new data
setInterval(function() {
  fetch('/data')
    .then(response => response.json())
    .then(data => updateTable(data));
}, 1000);  // Set the interval in milliseconds (e.g., 5000 ms = 5 seconds)
