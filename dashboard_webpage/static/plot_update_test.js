// plot_update.js

// Sample data for plotting (replace with your actual data)
var plotData = {
    labels: ['Label 1', 'Label 2', 'Label 3'],
    datasets: [
      {
        label: 'Plot 1',
        data: [10, 20, 30],
        backgroundColor: 'rgba(75, 192, 192, 0.2)',
        borderColor: 'rgba(75, 192, 192, 1)',
        borderWidth: 1
      },
      // Add more datasets as needed
    ]
  };
  
  // Function to update the plots on the webpage
  function updatePlots() {
    var plotsContainer = document.getElementById('plots-container');
    plotsContainer.innerHTML = '';  // Clear existing plots
  
    // Create a new canvas for each plot
    for (var i = 0; i < plotData.datasets.length; i++) {
      var canvas = document.createElement('canvas');
      canvas.id = 'plot-' + i;
      canvas.width = 400;  // Set your desired width
      canvas.height = 300; // Set your desired height
  
      // Set a title for each plot (replace with your actual titles)
      var title = document.createElement('h2');
      title.innerHTML = 'Plot Title ' + (i + 1);
  
      plotsContainer.appendChild(title);
      plotsContainer.appendChild(canvas);
  
      // Create the chart using Chart.js
      var ctx = canvas.getContext('2d');
      new Chart(ctx, {
        type: 'bar',  // Choose the chart type (e.g., 'bar', 'line', 'pie', etc.)
        data: plotData,
      });
    }
  }
  
  // Initial plots update
  updatePlots();
  