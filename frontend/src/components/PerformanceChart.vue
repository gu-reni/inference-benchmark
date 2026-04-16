<template>
  <div class="chart-container">
    <h3>推理耗时对比 (毫秒)</h3>
    <canvas ref="chartCanvas"></canvas>
  </div>
</template>

<script>
import { Chart, registerables } from 'chart.js'
Chart.register(...registerables)

export default {
  name: 'PerformanceChart',
  props: {
    cppTime: {
      type: Number,
      required: true
    },
    pythonTime: {
      type: Number,
      required: true
    }
  },
  mounted() {
    this.renderChart()
  },
  watch: {
    cppTime() {
      this.renderChart()
    },
    pythonTime() {
      this.renderChart()
    }
  },
  methods: {
    renderChart() {
      const ctx = this.$refs.chartCanvas.getContext('2d')
      
      // 销毁旧图表
      if (this.chart) {
        this.chart.destroy()
      }
      
      this.chart = new Chart(ctx, {
        type: 'bar',
        data: {
          labels: ['C++', 'Python'],
          datasets: [{
            label: '推理时间 (ms)',
            data: [this.cppTime, this.pythonTime],
            backgroundColor: [
              'rgba(54, 162, 235, 0.7)',
              'rgba(255, 99, 132, 0.7)'
            ],
            borderColor: [
              'rgba(54, 162, 235, 1)',
              'rgba(255, 99, 132, 1)'
            ],
            borderWidth: 1
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          scales: {
            y: {
              beginAtZero: true,
              title: {
                display: true,
                text: '耗时 (毫秒)'
              }
            }
          },
          plugins: {
            tooltip: {
              callbacks: {
                label: (context) => {
                  let label = context.dataset.label || ''
                  let value = context.parsed.y
                  return `${label}: ${value.toFixed(2)} ms`
                }
              }
            }
          }
        }
      })
    }
  },
  beforeUnmount() {
    if (this.chart) {
      this.chart.destroy()
    }
  }
}
</script>

<style scoped>
.chart-container {
  height: 300px;
  margin: 20px 0;
}
</style>