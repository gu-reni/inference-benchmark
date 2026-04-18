<template>
  <div class="chart-wrapper">
    <h3>⏱️ 推理耗时对比 (毫秒)</h3>
    <div class="chart-container">
      <canvas ref="chartCanvas"></canvas>
    </div>
    <div class="chart-footnote">
      加速比: <strong>{{ speedup.toFixed(2) }}x</strong> (C++ 比 Python 快)
    </div>
  </div>
</template>

<script>
import { Chart, registerables } from 'chart.js'
Chart.register(...registerables)

export default {
  name: 'PerformanceChart',
  props: {
    cppTime: { type: Number, required: true },
    pythonTime: { type: Number, required: true }
  },
  data() {
    return {
      chart: null
    }
  },
  computed: {
    speedup() {
      if (!this.pythonTime) return 0
      return this.pythonTime / this.cppTime
    }
  },
  watch: {
    cppTime() { this.renderChart() },
    pythonTime() { this.renderChart() }
  },
  mounted() {
    this.renderChart()
  },
  methods: {
    renderChart() {
      this.$nextTick(() => {
        const canvas = this.$refs.chartCanvas
        if (!canvas) return
        const ctx = canvas.getContext('2d')
        if (this.chart) this.chart.destroy()
        this.chart = new Chart(ctx, {
          type: 'bar',
          data: {
            labels: ['C++ (ONNX Runtime)', 'Python (PyTorch)'],
            datasets: [{
              label: '推理耗时 (ms)',
              data: [this.cppTime, this.pythonTime],
              backgroundColor: ['rgba(59, 130, 246, 0.8)', 'rgba(239, 68, 68, 0.8)'],
              borderColor: ['#2563eb', '#dc2626'],
              borderWidth: 2,
              borderRadius: 8,
              barPercentage: 0.6
            }]
          },
          options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
              tooltip: {
                callbacks: {
                  label: (context) => `${context.dataset.label}: ${context.parsed.y.toFixed(2)} ms`
                }
              },
              legend: { display: false }
            },
            scales: {
              y: {
                beginAtZero: true,
                grid: { color: '#e2e8f0' },
                title: {
                  display: true,
                  text: '耗时 (ms)',
                  color: '#64748b'
                }
              },
              x: {
                grid: { display: false }
              }
            }
          }
        })
      })
    }
  },
  beforeUnmount() {
    if (this.chart) this.chart.destroy()
  }
}
</script>

<style scoped>
.chart-wrapper {
  background: #ffffff;
  border-radius: 20px;
  padding: 1.5rem;
  margin: 1.5rem 0;
  box-shadow: inset 0 2px 4px rgba(0,0,0,0.02);
}
.chart-wrapper h3 {
  margin-bottom: 1.5rem;
  font-weight: 600;
  color: #1e293b;
}
.chart-container {
  height: 260px;
}
.chart-footnote {
  text-align: center;
  margin-top: 1.5rem;
  font-size: 1.1rem;
  color: #475569;
}
.chart-footnote strong {
  color: #2563eb;
  font-size: 1.3rem;
}
</style>