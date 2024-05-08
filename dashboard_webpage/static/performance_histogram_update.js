// Define the function to update performance plots
function updatePerformanceHistograms(data) {
    var plotsContainer = document.getElementById('performance-histograms');
    if (!plotsContainer) {
        console.error("Performance plots container not found.");
        return;
    }

    // Call the function to create or update performance plots with the provided data
    fillPerformanceHistograms(data, plotsContainer);
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


function fillPerformanceHistograms(data, plotsContainer) {
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
        var containerId = 'hist_' + plotName.replace(/ /g, '_'); // Generate unique container ID

        // Check if plot with container ID already exists
        var existingPlot = document.getElementById(containerId);
        if (existingPlot) {
            // Update existing plot with new difference data
            updateHistogramPlot(containerId, plotName, [difference]);
        } else {
            // Create a new plot if it doesn't exist
            createHistogramPlot(plotsContainer, plotName, [difference]);
            updateHistogramPlot(containerId, plotName, [difference]);
        }
    });
}


// Function to create histogram plots with dynamic binning
function createHistogramPlot(plotsContainer, title, data) {
    // Create a new plot container
    var plotContainer = document.createElement('div');

    // Set a unique ID for the container
    var containerId = 'hist_' + title.replace(/ /g, '_'); // Generate unique container ID
    plotContainer.id = containerId;

    // Append the plot container to the "plots-container"
    plotsContainer.appendChild(plotContainer);

    // Create the trace data for Plotly
    var plotlyTrace = {
        x: data,
        type: 'histogram',
        name: title,
    };

    // Create the layout for Plotly
    var layout = {
        title: title,
        xaxis: {
            title: 'Time (microseconds)',
        },
        yaxis: {
            title: 'Frequency',
        },
    };

    // Create the plot using Plotly
    Plotly.newPlot(containerId, [plotlyTrace], layout);
}

function updateHistogramPlot(containerId, title, newData) {
    // Get the Plotly plot container by its ID
    var plotContainer = document.getElementById(containerId);

    // Check if the plot with the ID already exists
    if (plotContainer) {
        // Get the existing data from the plot
        var existingData = plotContainer.data[0].x;

        // Combine the existing data with the new data
        var combinedData = existingData.concat(newData);

        // Update the existing plot with the combined data using Plotly
        Plotly.update(plotContainer, {
            x: [combinedData],
            type: 'histogram',
            name: title,
        });
    } else {
        console.error('Plot container not found for ID:', containerId);
    }
}

// Initial data update
fetch('/data')
    .then(response => response.json())
    .then(data => updatePerformanceHistograms(data));

// SocketIO event handler for data updates
socket.on('update_data', updatePerformanceHistograms);

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
                if (!isDataEqual(data, lastFetchedData)) { // Check if the fetched data is different from the last fetched data
                    updatePerformanceHistograms(data); // If data is different, update the plots
                    lastFetchedData = data; // Update the last fetched data
                } else {
                    console.log('Fetched data is the same as the last fetched data.');
                }
            }
        })
        .catch(error => {
            console.error('Error fetching initial data:', error.message);
        });
}, 200);  // Set the interval in milliseconds (e.g., 1000 ms = 1 second)
