<template>
  <div id="app">
    <header class="hero">
      <h1>🚀 推理性能对比</h1>
      <p class="subtitle">C++ ONNX Runtime vs Python PyTorch</p>
    </header>

    <main class="container">
      <div class="upload-card">
        <div class="upload-area" @click="$refs.fileInput.click()">
          <input
            ref="fileInput"
            type="file"
            accept="image/*"
            @change="handleFileUpload"
            style="display: none"
          />
          <div v-if="!imagePreview" class="upload-placeholder">
            <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M7 16a4 4 0 01-.88-7.903A5 5 0 1115.9 6L16 6a5 5 0 011 9.9M15 13l-3-3m0 0l-3 3m3-3v12" />
            </svg>
            <span>点击或拖拽上传图片</span>
            <small>支持 JPG、PNG 格式，建议尺寸 224x224</small>
          </div>
          <img v-else :src="imagePreview" class="preview-image" />
        </div>
        <button class="compare-btn" @click="runBenchmark" :disabled="!uploadedImage || loading">
          <span v-if="!loading">开始对比</span>
          <span v-else>推理中...</span>
        </button>
      </div>

      <div v-if="cppResult && pythonResult" class="results-section">
        <div class="cards-grid">
          <div class="result-card cpp">
            <div class="card-header">
              <span class="badge">C++</span>
              <span class="time">{{ (cppResult.inference_time_us / 1000).toFixed(2) }} ms</span>
            </div>
            <div class="speed-indicator">
              <div class="bar" :style="{ width: speedRatio + '%' }"></div>
            </div>
            <div class="tag">⚡ 基准性能</div>
          </div>

          <div class="result-card python">
            <div class="card-header">
              <span class="badge">Python</span>
              <span class="time">{{ (pythonResult.inference_time_us / 1000).toFixed(2) }} ms</span>
            </div>
            <div class="speed-indicator">
              <div class="bar" :style="{ width: '100%' }"></div>
            </div>
            <div class="tag">🐍 对比基准</div>
          </div>
        </div>

        <PerformanceChart
          :cppTime="cppResult.inference_time_us / 1000"
          :pythonTime="pythonResult.inference_time_us / 1000"
        />

        <div class="results-table-wrapper">
          <h3>📊 推理结果对比</h3>
          <table class="results-table">
            <thead>
              <tr>
                <th>排名</th>
                <th>C++ 预测类别</th>
                <th>C++ 置信度</th>
                <th>Python 预测类别</th>
                <th>Python 置信度</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="i in 5" :key="i">
                <td class="rank">#{{ i }}</td>
                <td>{{ getLabel(cppTopIndices[i-1]) }}</td>
                <td>{{ formatConfidence(cppTopProbs[i-1]) }}</td>
                <td>{{ getLabel(pythonTopIndices[i-1]) }}</td>
                <td>{{ formatConfidence(pythonTopProbs[i-1]) }}</td>
              </tr>
            </tbody>
          </table>
          <p class="footnote">注：置信度已通过 Softmax 归一化，因浮点运算差异略有不同属正常现象</p>
        </div>
      </div>
    </main>
  </div>
</template>

<script>
import PerformanceChart from './components/PerformanceChart.vue'
import axios from 'axios'

export default {
  name: 'App',
  components: { PerformanceChart },
  data() {
    return {
      uploadedImage: null,
      imagePreview: null,
      cppResult: null,
      pythonResult: null,
      loading: false,
      imagenetClasses: [],
      cppTopIndices: [],
      pythonTopIndices: [],
      cppTopProbs: [],
      pythonTopProbs: []
    }
  },
  computed: {
    speedRatio() {
      if (!this.cppResult || !this.pythonResult) return 0
      return (this.cppResult.inference_time_us / this.pythonResult.inference_time_us) * 100
    }
  },
  mounted() {
    this.loadImageNetClasses()
  },
  methods: {
    async loadImageNetClasses() {
      try {
        const response = await fetch('/imagenet_classes.json')
        this.imagenetClasses = await response.json()
      } catch (error) {
        console.error('Failed to load ImageNet classes:', error)
        this.imagenetClasses = []
      }
    },
    getLabel(index) {
      if (this.imagenetClasses.length > 0 && index >= 0 && index < this.imagenetClasses.length) {
        return this.imagenetClasses[index]
      }
      return `类别 ${index}`
    },
    softmax(logits) {
      const max = Math.max(...logits)
      const exp = logits.map(x => Math.exp(x - max))
      const sum = exp.reduce((a, b) => a + b, 0)
      return exp.map(x => x / sum)
    },
    formatConfidence(value) {
      return (value * 100).toFixed(2) + '%'
    },
    handleFileUpload(event) {
      const file = event.target.files[0]
      if (!file) return
      this.uploadedImage = file
      this.imagePreview = URL.createObjectURL(file)
      this.cppResult = null
      this.pythonResult = null
    },
    async runBenchmark() {
      if (!this.uploadedImage) return
      this.loading = true

      const buffer = await this.uploadedImage.arrayBuffer()
      const uint8Array = new Uint8Array(buffer)

      try {
        const [cppRes, pythonRes] = await Promise.all([
          axios.post('/api/cpp/inference', uint8Array, {
            headers: { 'Content-Type': 'application/octet-stream' }
          }),
          axios.post('/api/python/inference', uint8Array, {
            headers: { 'Content-Type': 'application/octet-stream' }
          })
        ])

        this.cppResult = cppRes.data
        this.pythonResult = pythonRes.data

        this.cppTopIndices = cppRes.data.top5_indices
        this.cppTopProbs = this.softmax(cppRes.data.top5_values)
        this.pythonTopIndices = pythonRes.data.top5_indices
        this.pythonTopProbs = this.softmax(pythonRes.data.top5_values)
      } catch (error) {
        console.error('请求失败:', error)
        alert('推理服务请求失败，请检查后端是否启动')
      } finally {
        this.loading = false
      }
    }
  }
}
</script>

<style>
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

body {
  font-family: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  min-height: 100vh;
  color: #1e293b;
}

#app {
  padding: 2rem 1rem;
}

.hero {
  text-align: center;
  margin-bottom: 3rem;
}

.hero h1 {
  font-size: 3rem;
  font-weight: 700;
  color: white;
  text-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
  letter-spacing: -0.025em;
}

.subtitle {
  font-size: 1.2rem;
  color: rgba(255, 255, 255, 0.9);
  margin-top: 0.5rem;
}

.container {
  max-width: 1000px;
  margin: 0 auto;
}

.upload-card {
  background: white;
  border-radius: 24px;
  padding: 2rem;
  box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.25);
  margin-bottom: 2rem;
}

.upload-area {
  border: 3px dashed #cbd5e1;
  border-radius: 16px;
  padding: 3rem 2rem;
  text-align: center;
  cursor: pointer;
  transition: all 0.2s;
}

.upload-area:hover {
  border-color: #667eea;
  background: #f8fafc;
}

.upload-placeholder {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 0.75rem;
  color: #64748b;
}

.upload-placeholder svg {
  stroke: #94a3b8;
}

.upload-placeholder span {
  font-size: 1.25rem;
  font-weight: 500;
  color: #334155;
}

.upload-placeholder small {
  font-size: 0.875rem;
}

.preview-image {
  max-width: 100%;
  max-height: 300px;
  border-radius: 12px;
  object-fit: contain;
}

.compare-btn {
  display: block;
  width: 100%;
  margin-top: 1.5rem;
  padding: 1rem;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  border: none;
  border-radius: 12px;
  font-size: 1.2rem;
  font-weight: 600;
  cursor: pointer;
  transition: transform 0.1s, box-shadow 0.1s;
}

.compare-btn:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 10px 20px -5px rgba(102, 126, 234, 0.4);
}

.compare-btn:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.results-section {
  background: white;
  border-radius: 24px;
  padding: 2rem;
  box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.25);
}

.cards-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 1.5rem;
  margin-bottom: 2.5rem;
}

.result-card {
  padding: 1.5rem;
  border-radius: 16px;
  background: #f8fafc;
}

.result-card.cpp {
  border-left: 6px solid #3b82f6;
}

.result-card.python {
  border-left: 6px solid #ef4444;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 1rem;
}

.badge {
  font-size: 1.25rem;
  font-weight: 700;
  padding: 0.25rem 0.75rem;
  border-radius: 30px;
  background: white;
  box-shadow: 0 2px 4px rgba(0,0,0,0.05);
}

.result-card.cpp .badge {
  color: #2563eb;
}

.result-card.python .badge {
  color: #dc2626;
}

.time {
  font-size: 1.5rem;
  font-weight: 700;
  font-family: 'JetBrains Mono', monospace;
}

.speed-indicator {
  height: 8px;
  background: #e2e8f0;
  border-radius: 10px;
  margin: 1rem 0;
  overflow: hidden;
}

.speed-indicator .bar {
  height: 100%;
  background: linear-gradient(90deg, #10b981, #34d399);
  border-radius: 10px;
  transition: width 0.5s ease;
}

.result-card.python .speed-indicator .bar {
  background: #94a3b8;
}

.tag {
  text-align: right;
  font-size: 0.9rem;
  color: #64748b;
}

.results-table-wrapper {
  margin-top: 2.5rem;
}

.results-table-wrapper h3 {
  margin-bottom: 1rem;
  font-weight: 600;
  color: #1e293b;
}

.results-table {
  width: 100%;
  border-collapse: collapse;
  border-radius: 12px;
  overflow: hidden;
  box-shadow: 0 4px 6px -4px rgba(0,0,0,0.1);
}

.results-table th {
  background: #f1f5f9;
  padding: 1rem;
  text-align: left;
  font-weight: 600;
  color: #475569;
}

.results-table td {
  padding: 1rem;
  border-bottom: 1px solid #e2e8f0;
}

.results-table tbody tr:last-child td {
  border-bottom: none;
}

.rank {
  font-weight: 600;
  color: #64748b;
}

.footnote {
  margin-top: 1rem;
  font-size: 0.875rem;
  color: #94a3b8;
  text-align: center;
}

@media (max-width: 640px) {
  .hero h1 {
    font-size: 2rem;
  }
  .cards-grid {
    grid-template-columns: 1fr;
  }
}
</style>