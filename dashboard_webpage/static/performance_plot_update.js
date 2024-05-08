// Define the function to update performance plots
function updatePerformancePlots(data) {
    var plotsContainer = document.getElementById('performance-plots');
    if (!plotsContainer) {
        console.error("Performance plots container not found.");
        return;
    }

    // Call the function to create or update performance plots with the provided data
    fillPerformancePlots(data, plotsContainer);
}

function calculateDifferences(lastObject, keys, plotNames) {
    var differences = [];

    for (var i = 0; i < keys.length - 1; i++) {
        // Get the values for the current and next keys
        var currentValue = parseFloat(lastObject[keys[i]]);
        var nextValue = parseFloat(lastObject[keys[i + 1]]);
        plotNames[i] = keys[i+1] + ' - ' + keys[i];
        if (nextValue === 0) {
            differences.push(null);
        }
        else if (currentValue === 0) {
            // Find the last nonzero value for timing comparison
            var j = i;
            while (j > 0) {
                currentValue = parseFloat(lastObject[keys[j-1]]);
                if (currentValue > 0) {
                    break;
                }
                j--;
            }
            // If we didn't find a nonzero value push null, otherwise push the correct value
            if (j > 0) {
                differences.push(nextValue - currentValue);
                plotNames[i] = keys[i+1] + ' - ' + keys[j-1];
            } else {
                differences.push(null);
            }
        }
        else if (!isNaN(currentValue) && !isNaN(nextValue)) {
            differences.push(nextValue - currentValue);
        } 
        else {
            differences.push(null); // If either value is not a number, push null
        }
    }

    return differences;
}


function fillPerformancePlots(data, plotsContainer) {
    var keysForPlots = [
        'TCPProcUnlocked', 'gotTCPHeaderWord', 'gotTCPHeaderWord2', 'GPUProcUnlocked', 
        'GPUProcDone', 'GPUCopyDone', 'MFEProcUnlocked', 'MFEBanksMade', 'losslessComp'
    ];
    var plotNames = [
        'gotTCPHeaderWord - TCPProcUnlocked', 'gotTCPHeaderWord2 - gotTCPHeaderWord', 
        'GPUProcUnlocked - gotTCPHeaderWord2', 'GPUProcDone - GPUProcUnlocked', 'GPUCopyDone - GPUProcDone', 
        'MFEProcUnlocked - GPUCopyDone', 'MFEBanksMade - MFEProcUnlocked', 'losslessComp - MFEBanksMade'
    ]; //Initialize plot names, but they will be overwritten.
    var lastObject = getLastObjectByChannel(data, "PERF");
    var differences = calculateDifferences(lastObject, keysForPlots, plotNames);

    // Create or update Plotly plots
    differences.forEach(function(difference, index) {
        if (difference === null) {
            // Skip this iteration if difference is null
            return;
        }
        var plotName = plotNames[index];
        var containerId = 'plot_' + plotName.replace(/ /g, '_'); // Generate unique container ID

        // Check if plot with container ID already exists
        var existingPlot = document.getElementById(containerId);
        var currentTime = new Date().toLocaleTimeString();
        if (existingPlot) {
            // Update existing plot with new difference data
            updatePlot(containerId, difference, currentTime, plotName);
        } else {
            // Create a new plot if it doesn't exist
            createNewPlot(plotsContainer, containerId, plotName);
            updatePlot(containerId, difference, currentTime, plotName);
        }
    });
}


// Function to create a new Plotly plot
function createNewPlot(plotsContainer, containerId, plotName) {
    var plotElement = document.createElement('div');
    plotElement.id = containerId; // Use the provided containerId
    plotsContainer.appendChild(plotElement);
    
    // Define the layout with axis labels
    var layout = {
        title: plotName,
        xaxis: {
            title: 'Time Stamp',
        },
        yaxis: {
            title: 'Time Taken [microseconds]',
        },
    };

    return Plotly.newPlot(containerId, [{ x: [], y: [], name: plotName }], layout);
}


//Upates the plotly plot
function updatePlot(containerId, newData, newTime, title) {
    // Retrieve existing plot data
    var plot = document.getElementById(containerId);
    var existingX = plot.data[0].x;
    var existingY = plot.data[0].y;

    // Update data arrays with new data (LIFO style)
    existingX = existingX.concat(newTime); // Concatenate new time to existing x-axis data
    existingY = existingY.concat(newData); // Concatenate new data to existing y-axis data

    // Limit the data length to 100 by removing oldest data points (LIFO)
    var MAX_LENGTH = 30;
    if (existingX.length > MAX_LENGTH) {
        var removeCount = existingX.length - MAX_LENGTH;
        existingX.splice(0, removeCount);
        existingY.splice(0, removeCount);
    }

    // Update the existing plot with new data using Plotly
    Plotly.update(containerId, {
        x: [existingX],
        y: [existingY],
        name: title
    });
}


// Initial data update
fetch('/data')
    .then(response => response.json())
    .then(data => updatePerformancePlots(data));

// SocketIO event handler for data updates
socket.on('update_data', updatePerformancePlots);

// Variable to store the last fetched data
var lastFetchedData = null; 

// Periodically fetch new data and update performance plots
setInterval(function() {
    fetch('/data')
        .then(response => {
            if (!response.ok) {
                throw new Error('Network response was not ok');
            }
            return response.json();
        })
        .then(data => {
            if (Object.keys(data).length === 0) {
                console.log('No data fetched.');
            } else {
                if (!(JSON.stringify(lastFetchedData) === JSON.stringify(data))) { // Check if the fetched data is different from the last fetched data
                    updatePerformancePlots(data); // If data is different, update the plots
                    lastFetchedData = data; // Update the last fetched data
                } else {
                    console.log('Fetched data is the same as the last fetched data.');
                }
            }
        })
        .catch(error => {
            console.error('Error fetching initial data:', error.message);
        });
}, 1000);  // Set the interval in milliseconds (e.g., 1000 ms = 1 second)
