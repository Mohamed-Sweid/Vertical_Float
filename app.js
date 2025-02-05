
// const csvUrl = 'https://api.allorigins.win/get?url=https://docs.google.com/spreadsheets/d/1Jowav3RktAQIUirp8BHtn8AI-6eG-cNpNwwp2gd43vU/gviz/tq?tqx=out:csv&sheet=Sheet1';
// Original Google Sheets CSV URL (published version)
// const sheetUrl = 'https://docs.google.com/spreadsheets/d/1Jowav3RktAQIUirp8BHtn8AI-6eG-cNpNwwp2gd43vU/gviz/tq?tqx=out:csv&sheet=Sheet1';

// Encode URL and add proxy
// Replace with your published Google Sheets CSV URL
const SHEET_URL = 'https://docs.google.com/spreadsheets/d/e/2PACX-1vQTwkWR18z4rqGOIVEBMpqwd3PdpymCgFMwTiZMDzfKkunyR42Zy74JIr20LDvTRipzql6QqneQnOZv/pub?gid=0&single=true&output=csv';
const CHART_COLORS = {
  red: 'rgb(255, 99, 132)',
  orange: 'rgb(255, 159, 64)',
  yellow: 'rgb(255, 205, 86)',
  green: 'rgb(75, 192, 192)',
  blue: 'rgb(54, 162, 235)',
  purple: 'rgb(153, 102, 255)',
  pink: 'rgb(255, 99, 255)'
};

// Performance optimizations
Chart.defaults.animation = false;
Chart.defaults.elements.line.borderWidth = 1;
Chart.defaults.plugins.legend.display = false;
// Register the zoom plugin
Chart.register(ChartZoom);

async function loadData() {
  try {
      showLoading();
      const response = await fetch(SHEET_URL);
      if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
      
      const csvData = await response.text();
      const parsedData = parseCSV(csvData);
      const processedData = processData(parsedData);
      
      createAllCharts(processedData);
  } catch (error) {
      showError(error);
  } finally {
      hideLoading();
  }
}

function parseCSV(csvData) {
  return Papa.parse(csvData, {
      header: true,
      dynamicTyping: true,
      skipEmptyLines: true,
      transformHeader: header => header.trim()
  }).data;
}

function processData(data) {
  console.log(data);
  return data
      .map(row => ({
          ...row,
          Time: parseDate(row.H) // Replace 'H' with your time column header
      }))
      .filter(row => isValidDate(row.Time) && isValidRow(row));
}

function parseDate(dateString) {
  // Match the exact date format with time
  const date = moment(dateString, 'YYYY-MM-DD H:mm:ss');
  // console.log(date.isValid() ? date.toDate() : null);
  return date.isValid() ? date.toDate() : null;
}

function isValidDate(date) {
  return date && date instanceof Date && !isNaN(date.getTime());
}

function isValidRow(row) {
  return Object.values(row).every(value => {
    // Allow: numbers, valid dates, and null/undefined
    return typeof value === 'number' || 
           value instanceof Number || 
           value !== null ||
           value === undefined;
  });
}

function createAllCharts(data) {
  const dashboard = document.getElementById('dashboard');
  const colorKeys = Object.keys(CHART_COLORS);
  const columnsToPlot = ['A', 'B', 'C', 'D', 'E', 'F', 'G']; // Update these
  
  columnsToPlot.forEach((col, index) => {
      const chartCard = createChartCard(col);
      dashboard.appendChild(chartCard);
      createChart(chartCard, col, data, colorKeys[index % colorKeys.length]);
  });
}

function createChartCard(title) {
  const card = document.createElement('div');
  card.className = 'chart-card';
  card.innerHTML = `<div class="chart-title">${title} Over Time</div>`;
  return card;
}

function createChart(container, column, data, colorKey) {
  const chartWrapper = document.createElement('div');
  chartWrapper.style.height = '400px';
  chartWrapper.style.position = 'relative';
  
  // Add control toolbar
  const toolbar = document.createElement('div');
  toolbar.className = 'chart-toolbar';
  toolbar.innerHTML = `
      <button class="zoom-btn" title="Zoom"><i class="fas fa-search-plus"></i></button>
      <button class="pan-btn" title="Pan"><i class="fas fa-hand-paper"></i></button>
      <button class="reset-btn" title="Reset Zoom"><i class="fas fa-home"></i></button>
  `;

  const canvas = document.createElement('canvas');
  chartWrapper.appendChild(canvas);
  chartWrapper.appendChild(toolbar);
  container.appendChild(chartWrapper);

  const yValues = data.map(d => d[column]);
  const timeData = data.map(d => d.Time);
  
  const chart = new Chart(canvas, getChartConfig(column, yValues, timeData, CHART_COLORS[colorKey]));
  // Add control handlers
  let currentMode = 'zoom';
  let isPanning = false;
  let lastPos = null;
    
  toolbar.querySelector('.zoom-btn').addEventListener('click', () => {
      currentMode = 'zoom';
      chart.options.plugins.zoom.zoom.enabled = true;
      chart.options.plugins.zoom.pan.enabled = false;
  });
  // Pan button click handler
  toolbar.querySelector('.pan-btn').addEventListener('click', () => {
    chart.options.plugins.zoom.zoom.enabled = false;
    chart.options.plugins.zoom.pan.enabled = false;
    chart.update();
    toggleActiveButton(toolbar, 'pan');
  });
    // Canvas mouse handlers
  canvas.addEventListener('mousedown', (e) => {
    if (chart.options.plugins.zoom.pan.enabled === false) {
        isPanning = true;
        lastPos = {
            x: e.offsetX,
            y: e.offsetY
        };
        canvas.style.cursor = 'grabbing';
    }
  });
  canvas.addEventListener('mousemove', (e) => {
    if (isPanning && lastPos) {
        const deltaX = e.offsetX - lastPos.x;
        const deltaY = e.offsetY - lastPos.y;

        // Convert pixel delta to data units
        const xScale = chart.scales.x;
        const yScale = chart.scales.y;
        
        const xMin = xScale.min - (deltaX / xScale.width) * (xScale.max - xScale.min);
        const xMax = xScale.max - (deltaX / xScale.width) * (xScale.max - xScale.min);
        
        const yMin = yScale.min + (deltaY / yScale.height) * (yScale.max - yScale.min);
        const yMax = yScale.max + (deltaY / yScale.height) * (yScale.max - yScale.min);

        // Update scale limits
        xScale.min = xMin;
        xScale.max = xMax;
        yScale.min = yMin;
        yScale.max = yMax;

        chart.update('none');
        lastPos = {
            x: e.offsetX,
            y: e.offsetY
        };
    }
  });
  canvas.addEventListener('mouseup', () => {
    isPanning = false;
    lastPos = null;
    canvas.style.cursor = 'grab';
    chart.update();
  });

  canvas.addEventListener('mouseleave', () => {
    if (isPanning) {
        isPanning = false;
        lastPos = null;
        canvas.style.cursor = 'grab';
        chart.update();
    }
  });

  toolbar.querySelector('.reset-btn').addEventListener('click', () => {
      chart.resetZoom();
  });
  setupResizeHandler(chartWrapper, chart);
  canvas.style.cursor = 'grab';
  return chart;
}

function getChartConfig(title, data, labels, color) {
  return {
      type: 'line',
      data: {
          labels: labels,
          datasets: [{
              label: title,
              data: data,
              borderColor: color,
              backgroundColor: color + '20',
              tension: 0.1
          }]
      },
      options: getChartOptions(data)
  };
}

function getChartOptions(data) {
  const yBounds = calculateSafeBounds(data.filter(v => typeof v === 'number'));
  
  return {
      responsive: true,
      maintainAspectRatio: false,
      scales: {
          x: {
              type: 'time',
              bounds: 'data',
              time: {
                  tooltipFormat: 'MMM D',
                  unit: 'day'
              },
              grid: { display: false }
          },
          y: {
              min: yBounds.min,
              max: yBounds.max,
              grace: '5%',
              ticks: {
                  autoSkip: true,
                  maxTicksLimit: 5,
                  callback: value => Number(value.toFixed(2)).toLocaleString()
              },
              grid: { color: '#f0f0f0' }
          }
      },
      plugins: {
          tooltip: {
              callbacks: {
                  label: context => `${context.dataset.label}: ${Number(context.parsed.y.toFixed(2)).toLocaleString()}`
              }
          },
          decimation: {
              enabled: true,
              algorithm: 'lttb'
          },
          zoom: {
            zoom: {
                wheel: {
                    enabled: true,
                },
                pinch: {
                    enabled: true
                },
                mode: 'xy',
                speed: 100
            },
            pan: {
                enabled: true,
                mode: 'xy',
                speed: 100
            },
            limits: {
                x: { min: 'original', max: 'original' },
                y: { min: 'original', max: 'original' }
            }
        }
    }
};
}

function calculateSafeBounds(values) {
  // console.log(values);
  if (values.length === 0) return { min: 0, max: 1 };
  const min = Math.min(...values);
  const max = Math.max(...values);
  const range = max - min || Math.abs(min) || 1;
  return { min: min - range * 0.05, max: max + range * 0.05 };
}

function setupResizeHandler(container, chart) {
  const resizeObserver = new ResizeObserver(() => chart.resize());
  resizeObserver.observe(container);
}

function showLoading() {
  document.body.innerHTML += `
      <div id="loading" style="...">
          Loading data from Google Sheets...
      </div>
  `;
}

function hideLoading() {
  const loading = document.getElementById('loading');
  if (loading) loading.remove();
}

function showError(error) {
  const errorDiv = document.createElement('div');
  errorDiv.style.color = 'red';
  errorDiv.style.padding = '20px';
  errorDiv.innerHTML = `Error: ${error.message}`;
  document.body.prepend(errorDiv);
}
function toggleActiveButton(toolbar, activeMode) {
  toolbar.querySelectorAll('button').forEach(btn => {
      btn.classList.remove('active');
  });
  toolbar.querySelector(`.${activeMode}-btn`).classList.add('active');
}
// Initialize
loadData();