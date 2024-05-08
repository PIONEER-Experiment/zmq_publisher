// performance_table_update.js

var socket = io.connect('http://' + document.domain + ':' + location.port);

function formatValue(key, data) {
    if (key === 'cdfHeader') {
        return '0x' + Number(data).toString(16); // Display cdfHeader in hexadecimal format
    }
    else if (keysToDisplay.indexOf(key) >= 5) {
        return (data !== 0 ? (() => {
            const dateString = new Date(data / 1000).toLocaleString();
            const index = dateString.lastIndexOf(':')+3;
            const microseconds = String(data % 1000000).padStart(6, '0');
            return dateString.slice(0, index) + '.' + microseconds + ' ' + dateString.slice(index + 1);
        })() : 'N/A');    }
    else {
        return data; // Display the value as it is for the first 5 keys
    }
}


// List of keys to display in the table and their corresponding labels
var keysToDisplay = [
    'crateNum', 'cdfHeader', 'MidasFillNum', 'TCPFillNum', 'GPUFillNum',
    'TCPProcUnlocked', 'gotTCPHeaderWord', 'gotTCPHeaderWord2', 'GPUProcUnlocked', 'GPUProcDone', 'GPUCopyDone', 'MFEProcUnlocked', 'MFEBanksMade', 'losslessComp'
];
var labels = [
    'crateNum', 'cdfHeader', 'MidasFillNum', 'TCPFillNum', 'GPUFillNum',
    'TCPProcUnlocked', 'gotTCPHeaderWord', 'gotTCPHeaderWord2', 'GPUProcUnlocked', 'GPUProcDone', 'GPUCopyDone', 'MFEProcUnlocked', 'MFEBanksMade', 'losslessComp'
];

// Function to update the performance table (regular timings)
function updatePerformanceTable(data) {
    var tableBody = document.getElementById('perf-table-body');
    tableBody.innerHTML = ''; // Clear existing rows
    var lastObject = getLastObjectByChannel(data, "PERF")

    var row = tableBody.insertRow();

    // Populate the row cells with channel information
    keysToDisplay.forEach(function(key) {
        var cell = row.insertCell();
        var value = formatValue(key, lastObject[key]);
        cell.innerText = value;
    });
}

//Function to update difference table (difference in timings)
function updateDifferenceTable(data) {
    var tableBody = document.getElementById('diff-table-body');
    tableBody.innerHTML = ''; // Clear existing rows

    var rowLabels = ['TCPProcUnlocked', 'gotTCPHeaderWord', 'gotTCPHeaderWord2', 'GPUProcUnlocked', 'GPUProcDone', 'GPUCopyDone', 'MFEProcUnlocked', 'MFEBanksMade', 'losslessComp'];
    var columnLabels = ['TCPProcUnlocked', 'gotTCPHeaderWord', 'gotTCPHeaderWord2', 'GPUProcUnlocked', 'GPUProcDone', 'GPUCopyDone', 'MFEProcUnlocked', 'MFEBanksMade', 'losslessComp'];

    // Parse the "latest_content" from the "PERF" channel data
    var lastObject = getLastObjectByChannel(data, "PERF")
    if (lastObject !== null) {
        // Iterate through each row label
        rowLabels.forEach(function(rowLabel, rowIndex) {
            var row = tableBody.insertRow(); // Insert a new row

            // Create the first cell containing the row label
            var rowHeaderCell = document.createElement('th');
            rowHeaderCell.innerText = rowLabel;
            row.appendChild(rowHeaderCell);

            // Iterate through each column label
            var rowData = parseFloat(lastObject[rowLabel])
            columnLabels.forEach(function(columnLabel, columnIndex) {
                var columnData = parseFloat(lastObject[columnLabel]); // Get the column data from the provided data object

                // Create a cell for each column label and calculate the difference
                var cell = row.insertCell();
                cell.style.textAlign = 'center';
                cell.style.fontWeight = 'bold';
                if (columnIndex >= rowIndex) {
                    // Redudant cells are blacked out
                    cell.style.backgroundColor = 'black';
                }
                else if (columnData === 0 || rowData === 0) {
                    // If we don't have data for the row or the column
                    // display 'N/A' and color the cell gray
                    cell.innerText = 'N/A';
                    cell.style.backgroundColor = 'gray';
                } 
                else {
                    // Calculate the difference between row and column data
                    var difference = rowData - columnData;
                    if (difference <= 0) {
                        // If the difference is negative, black out the cell
                        cell.innerText = difference;
                        cell.style.backgroundColor = 'lightgray';
                    } else {
                        // Display the calculated difference and apply heatmap color
                        cell.innerText = difference;
                        // Apply heatmap color based on the magnitude of the difference
                        // Adjust the color scale as needed
                        cell.style.backgroundColor = heatMapColorForValue(difference);
                    }
                }
            });
        });
    } else {
        // Handle the case when no "PERF" channel data is found
        console.error("No 'PERF' channel data found.");
    }
}

// Function to generate heatmap color based on value
function heatMapColorForValue(value) {
    // Define the color scale boundaries
    var lowerBound = -50; // Lower bound of the color scale
    var upperBound = 50; // Upper bound of the color scale

    // Define the RGB values for different color stops
    var startColor = [0, 255, 0]; // Green
    var middleColor = [255, 255, 0]; // Yellow
    var endColor = [255, 0, 0]; // Red

    // Enforce the bounds of the value
    value = Math.max(Math.min(value, upperBound), lowerBound);

    // Calculate the percentage of value within the color scale
    var percentage;
    if (value < 0) {
        percentage = value / lowerBound;
    } else {
        percentage = value / upperBound;
    }

    // Interpolate the RGB values based on the percentage
    var color = [];
    for (var i = 0; i < 3; i++) {
        if (value < 0) {
            color[i] = Math.round((1 - percentage) * startColor[i] + percentage * middleColor[i]);
        } else {
            color[i] = Math.round((1 - percentage) * middleColor[i] + percentage * endColor[i]);
        }
    }

    // Convert the RGB values to hexadecimal color code
    var hexColor = '#' + color.map(c => c.toString(16).padStart(2, '0')).join('');

    return hexColor;
}


function updateTables(data) {
    updatePerformanceTable(data);
    updateDifferenceTable(data);
}

// Initial data update
fetch('/data')
  .then(response => response.json())
  .then(data => updateTables(data));

// SocketIO event handler for data updates
socket.on('update_data', updateTables);

// Periodically fetch new data
setInterval(function() {
  fetch('/data')
    .then(response => response.json())
    .then(data => updateTables(data));
}, 1000);  // Set the interval in milliseconds (e.g., 5000 ms = 5 seconds)
