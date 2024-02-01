// plot_update.js

var socket = io.connect('http://' + document.domain + ':' + location.port);

// Initialize generate histogram mode to false
var generateHistogramMode = true;

// Global variables for update frequencies in hertz
var traceUpdateFrequency = 1; // Default interval: 1 second for traces
var histUpdateFrequency = 10; // Default interval: .1 seconds for histograms

// Global variables to store the last update times
var lastTraceUpdate = 0;
var lastHistUpdate = 0;

// Function to check if it's time to update based on the specified frequency and last update time
function isReadyToUpdate(lastUpdate, updateFrequency) {
    var currentTime = Date.now();
    var elapsedTime = currentTime - lastUpdate;
    var updateInterval = 1000 / updateFrequency; // Convert frequency to milliseconds

    return elapsedTime >= updateInterval;
}

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

function addPlotSelectorOption(plotSelector, containerId, title, isSelected) {
    var option = document.createElement('option');
    option.value = containerId;
    option.text = title;

    // Apply the selected-option class if the option should be selected
    if (isSelected) {
        option.classList.add('selected-option');
        option.selected = true;
    }

    // Add the option to the plot selector
    plotSelector.add(option);
}
// Function to update the plots and current Run, sub-run, and event on the webpage
function updatePlots(data) {
    // Access the data and update the plots and current Run, sub-run, and event as needed
    var plotsContainer = document.getElementById('plots-container');
    var plotSelector = document.getElementById('plot-selector');
    var currentRunElement = document.getElementById('current-run');
    var currentSubRunElement = document.getElementById('current-subrun');
    var currentEventElement = document.getElementById('current-event');

    for (var channel in data) {
        var channelData = data[channel];

        if (channelData.channel_name === "DATA") {
            // Handle "DATA" channel data
            handleDataChannel(channelData, plotsContainer, plotSelector, currentRunElement, currentSubRunElement, currentEventElement, generateHistogramMode);
        } else if (channelData.channel_name === "HIST" && !generateHistogramMode) {
            // Handle "HIST" channel data
            handleHistChannel(channelData, plotsContainer, plotSelector);
        }
    }
}

function handleDataChannel(channelData, plotsContainer, plotSelector, currentRunElement, currentSubRunElement, currentEventElement, doGenerateHistograms) {
    try {
        var lastObject = getLastObject(channelData)

        // Extract current Run, sub-run, and event information
        var currentRun = lastObject.run;
        var currentSubRun = lastObject.subRun;
        var currentEvent = lastObject.event;

        // Update the content of the HTML elements for current Run, sub-run, and event
        currentRunElement.innerText = currentRun;
        currentSubRunElement.innerText = currentSubRun;
        currentEventElement.innerText = currentEvent;

        // Access the "waveforms" array
        var waveforms = lastObject.waveforms;

        // Loop through each waveform and create/update a plot
        var skipTraces = true;
        var skipHists = true;
        if (isReadyToUpdate(lastTraceUpdate, traceUpdateFrequency)) {
            // Perform trace update logic here
            skipTraces = false;
            // Update the last trace update time
            lastTraceUpdate = Date.now();
        }
        
        // Check if it's time to update histograms
        if (isReadyToUpdate(lastHistUpdate, histUpdateFrequency)) {
            // Perform histogram update logic here
            skipHists = false;
            // Update the last histogram update time
            lastHistUpdate = Date.now();
        }
        
        waveforms.forEach(function (waveform, index) {
            // Construct a title using information from the waveform
            var title = 'AMC Slot: ' + waveform.amcSlotNum + ', Channel: ' + waveform.channel +
                ', Channel ID: ' + waveform.channel_id + ', Channel Type: ' + waveform.channel_type + ', Crate Num: ' + waveform.crateNum;
            
            if (!skipTraces) {
                // Set a unique ID for the container
                var containerId = 'plot-' + title.replace(/[^a-zA-Z0-9]/g, '');

                // Check if the plot with the ID already exists
                var existingPlot = document.getElementById(containerId);
                if (existingPlot) {
                    // Update the existing plot with new data
                    updateWaveformPlot(containerId, title, waveform.trace);
                } else {
                    // Create a new plot if it doesn't exist
                    createWaveformPlot(plotsContainer, title, waveform.trace);
                    addPlotSelectorOption(plotSelector, containerId, title, true);
                }
            }

            // Check if we should generate histograms
            if (doGenerateHistograms && !skipHists) {
                // Calculate the sum of waveform values
                var waveformSum = waveform.trace.reduce((sum, value) => sum + value, 0);

                // Construct a title for the histogram
                var histTitle = title + ' - Sum Histogram';

                // Set a unique ID for the histogram container
                var histContainerId = 'hist-' + histTitle.replace(/[^a-zA-Z0-9]/g, '');

                // Check if the histogram with the ID already exists
                var existingHist = document.getElementById(histContainerId);

                if (existingHist) {
                    // Update the existing histogram with new data
                    updateHistogramPlot(histContainerId, histTitle, [waveformSum], [0, 1]); // Assuming bin edges [0, 1]
                } else {
                    // Create a new histogram if it doesn't exist
                    createHistogramPlot(plotsContainer, histTitle, [waveformSum], [0, 1]); // Assuming bin edges [0, 1]
                    addPlotSelectorOption(plotSelector, histContainerId, histTitle, true);
                }
            }
        });
    } catch (error) {
        // Display the error message in the "plots-container"
        plotsContainer.innerHTML += 'Error parsing latest_content for DATA channel: ' + error.message + '<br>';
    }
}
// Function to plot waveforms
function createWaveformPlot(plotsContainer, title, trace) {
    // Create a new plot container
    var plotContainer = document.createElement('div');
    plotContainer.className = 'plot-container'; // Apply the plot-container class

    // Set a unique ID for the container
    var containerId = 'plot-' + title.replace(/[^a-zA-Z0-9]/g, '');
    plotContainer.id = containerId;

    // Append the plot container to the "plots-container"
    plotsContainer.appendChild(plotContainer);

    // Create the trace data for Plotly
    var plotlyTrace = {
        x: Array.from({ length: trace.length }, (_, i) => i), // Assuming time is the index
        y: trace,
        type: 'scatter',
        mode: 'lines',
        name: title,
    };

    // Create the layout for Plotly
    var layout = {
        title: title,
        xaxis: {
            title: 'Time',
        },
        yaxis: {
            title: 'Voltage [arbitrary units]',
        },
    };

    // Create the plot using Plotly
    Plotly.newPlot(containerId, [plotlyTrace], layout);
}
function updateWaveformPlot(containerId, title, trace) {
    // Update the existing plot with new data using Plotly
    Plotly.update(containerId, {
        x: [Array.from({ length: trace.length }, (_, i) => i)],
        y: [trace],
        name: title
    });
}

// Function to create histogram plots with dynamic binning
function createHistogramPlot(plotsContainer, title, data) {
    // Create a new plot container
    var plotContainer = document.createElement('div');
    plotContainer.className = 'plot-container'; // Apply the plot-container class

    // Set a unique ID for the container
    var containerId = 'hist-' + title.replace(/[^a-zA-Z0-9]/g, '');
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
            title: 'Channel Sum',
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

function handleHistChannel(channelData, plotsContainer, plotSelector) {
    // Check if it's time to update histograms
    if (isReadyToUpdate(lastHistUpdate, histUpdateFrequency)) {
        lastHistUpdate = Date.now();
    } else {
        return;
    }
    try {
        var lastObject = getLastObject(channelData);
        // Iterate through each histogram in the 'latest_content'
        for (var histogramID in lastObject) {
            var histogramData = lastObject[histogramID];

            // Process only TH1D histograms
            if (histogramData._typename === "TH1D") {
                // Extract TH1D specific data
                var histName = histogramData.fName;
                var array = histogramData.fArray;
                var barOffset = histogramData.fBarOffset;
                var barWidth = histogramData.fBarWidth;

                // Extract bin-related information
                var numBins = array.length - 2; //Remove over and underflow bins
                var binEdges = []; // calculate bin edges based on barOffset, barWidth, and numBins

                // logic to calculate bin edges based on barOffset, barWidth, and numBins
                for (var i = 0; i < numBins; i++) {
                    binEdges.push(barOffset + i * barWidth);
                }
                var frequencies = array.slice(1, -1);

                // Set a unique ID for the container
                var containerId = 'hist-plot-' + histName.replace(/[^a-zA-Z0-9]/g, '');
                console.log('containerId:', containerId);

                // Check if the plot with the ID already exists
                var existingPlot = document.getElementById(containerId);

                if (existingPlot) {
                    // Update the existing histogram plot with new data
                    updateBarPlot(containerId, histName, frequencies, binEdges);
                } else {
                    // Create a new histogram plot if it doesn't exist
                    createBarPlot(plotsContainer, histName, frequencies, binEdges);
                    addPlotSelectorOption(plotSelector, containerId, histName, true);
                }
            }
        }
    } catch (error) {
        // Display the error message in the "plots-container"
        plotsContainer.innerHTML += 'Error handling histogram channel: ' + error.message + '<br>';
    }
}

// Function to create bar chart
function createBarPlot(plotsContainer, title, data, binEdges) {

    // Create a new plot container
    var plotContainer = document.createElement('div');
    plotContainer.className = 'plot-container'; // Apply the plot-container class

    // Set a unique ID for the container
    var containerId = 'hist-plot-' + title.replace(/[^a-zA-Z0-9]/g, '');
    plotContainer.id = containerId;

    // Append the plot container to the "plots-container"
    plotsContainer.appendChild(plotContainer);

    // Create the trace data for Plotly
    var plotlyTrace = {
        x: binEdges,
        y: data,
        type: 'bar',
        name: title,
    };

    // Create the layout for Plotly
    var layout = {
        title: title,
        xaxis: {
            title: 'X-axis Title',
        },
        yaxis: {
            title: 'Y-axis Title',
        },
    };

    // Create the plot using Plotly
    Plotly.newPlot(containerId, [plotlyTrace], layout);
}

// Function to update bar chart
function updateBarPlot(containerId, title, data, binEdges) {
    console.log('Data:', data);
    console.log('Bin Edges:', binEdges);

    // Update the existing plot with new data using Plotly
    Plotly.update(containerId, {
        x: [binEdges],
        y: [data],
        type: 'bar',
        name: title,
    });
}

// Function to toggle the visibility of all plots
function toggleAllPlots() {
  var plotsContainer = document.getElementById('plots-container');
  var plotSelector = document.getElementById('plot-selector');
  var options = plotSelector.options;

  // Get a list of all visible plots
  var visiblePlots = Array.from(plotsContainer.children).filter(plot => !plot.classList.contains('hidden'));

  if (visiblePlots.length === 0) {
      // If no plots are visible, toggle visibility for all plots
      for (var i = 0; i < options.length; i++) {
          togglePlotVisibility(options[i].value);
      }
  } else {
      // If some plots are visible, hide all plots
      for (var i = 0; i < options.length; i++) {
          hidePlot(options[i].value);
      }
  }
  togglePlotSelectorItemColor();
}

// Function to toggle the visibility of all trace plots
function toggleAllTracesPlots() {
    toggleAllPlotsByPrefix('plot-');
}

// Function to toggle the visibility of all histogram plots
function toggleAllHistsPlots() {
    toggleAllPlotsByPrefix('hist-');
    toggleAllPlotsByPrefix('hist-plot-');
}

// Function to toggle the visibility of all plots with a specific prefix
function toggleAllPlotsByPrefix(prefix) {
    var plotsContainer = document.getElementById('plots-container');
    var plotSelector = document.getElementById('plot-selector');
    var options = plotSelector.options;

    // Get a list of all visible plots with the specified prefix
    var visiblePlots = Array.from(plotsContainer.children).filter(plot => !plot.classList.contains('hidden') && plot.id.startsWith(prefix));

    if (visiblePlots.length === 0) {
        // If no plots with the specified prefix are visible, toggle visibility for all plots
        for (var i = 0; i < options.length; i++) {
            if (options[i].value.startsWith(prefix)) {
                togglePlotVisibility(options[i].value);
            }
        }
    } else {
        // If some plots with the specified prefix are visible, hide all plots with that prefix
        for (var i = 0; i < options.length; i++) {
            if (options[i].value.startsWith(prefix)) {
                hidePlot(options[i].value);
            }
        }
    }
    togglePlotSelectorItemColor();
}

// Function to toggle the visibility of selected plots
function toggleSelectedPlots() {
  var plotSelector = document.getElementById('plot-selector');
  var options = plotSelector.options;

  // Toggle visibility for selected plots based on the selected options
  for (var i = 0; i < options.length; i++) {
      var option = options[i];
      if (option.selected) {
          togglePlotVisibility(option.value);
      }
  }
  togglePlotSelectorItemColor();
}

// Function to toggle the visibility of a plot
function togglePlotVisibility(containerId) {
    var plotContainer = document.getElementById(containerId);
    if (plotContainer) {
        // Toggle the "hidden" class
        plotContainer.classList.toggle('hidden');
        var plotSelector = document.getElementById('plot-selector');
        var options = plotSelector.options;
        for (var i = 0; i < options.length; i++) {
            if (options[i].value === containerId) {
                options[i].classList.toggle('selected-option');
            }
        }
    }
}

// Function to toggle the color of a plot selector item
function togglePlotSelectorItemColor() {
    var plotSelector = document.getElementById('plot-selector');
    var options = plotSelector.options;

    // Remove the class from all options
    for (var i = 0; i < options.length; i++) {
        options[i].classList.remove('selected-option');
    }

    // Get a list of all visible plots
    var plotsContainer = document.getElementById('plots-container');
    var visiblePlots = Array.from(plotsContainer.children).filter(plot => !plot.classList.contains('hidden'));

    // Set the class for visible plots
    for (var i = 0; i < options.length; i++) {
        if (visiblePlots.some(plot => plot.id === options[i].value)) {
            options[i].classList.add('selected-option');
        }
    }
}
// Function to hide a plot
function hidePlot(containerId) {
    var plotContainer = document.getElementById(containerId);
    if (plotContainer) {
        // Add the "hidden" class to hide the plot
        plotContainer.classList.add('hidden');
    }
}
  

// Add this function to your plot_update.js
function setUpdateInterval() {
    var histUpdateFrequencyInput = document.getElementById('hist-update-frequency');
    var traceUpdateFrequencyInput = document.getElementById('trace-update-frequency');

    histUpdateFrequency = parseFloat(histUpdateFrequencyInput.value);
    traceUpdateFrequency = parseFloat(traceUpdateFrequencyInput.value);

    // Clear existing interval
    clearInterval(updatePlotsInterval);

    // Set a new interval with the updated frequency
    updatePlotsInterval = setInterval(updatePlotsData, (1 / getLCM(histUpdateFrequency,traceUpdateFrequency)) * 1000);
}

// Function to calculate the greatest common divisor (GCD) using Euclidean algorithm
function getGCD(a, b) {
    while (b !== 0) {
        var temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// Function to calculate the greatest common divisor (GCD) using Euclidean algorithm
function getLCM(a, b) {
    return (a*b) / getGCD(a,b);
}


// Function to toggle the histogram mode
function toggleHistogramMode() {
    generateHistogramMode = !generateHistogramMode;

    // Call the updatePlots function with the updated mode
    updatePlots(data);
}

function updatePlotsData() {
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
                updatePlots(data);
            }
        })
        .catch(error => {
            console.error('Error fetching data:', error.message);
        });
}

// Initial data update for plots
fetch('/data')
    .then(response => {
        if (!response.ok) {
            throw new Error('Network response was not ok');
        }
        return response.json();
    })
    .then(data => {
        if (Object.keys(data).length === 0) {
            console.log('No initial data fetched.');
        } else {
            updatePlots(data);
        }
    })
    .catch(error => {
        console.error('Error fetching initial data:', error.message);
    });

// SocketIO event handler for data updates
socket.on('update_data', updatePlots);

// Set the initial interval
var updatePlotsInterval = setInterval(updatePlotsData, (1/getLCM(histUpdateFrequency,traceUpdateFrequency))*1000);  // Default interval: 0.1 second

