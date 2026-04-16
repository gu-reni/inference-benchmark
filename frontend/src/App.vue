<template>
  <div id="app">
    <h1>🔥 C++ vs Python 推理性能对比</h1>
    
    <div class="upload-section">
      <input type="file" @change="handleFileUpload" accept="image/*" />
      <button @click="runBenchmark" :disabled="!uploadedImage">开始对比</button>
    </div>
    
    <div v-if="imagePreview" class="preview">
      <img :src="imagePreview" alt="Preview" />
    </div>
    
    <div v-if="cppResult && pythonResult" class="results">
      <PerformanceChart 
        :cppTime="cppResult.inference_time_us / 1000"
        :pythonTime="pythonResult.inference_time_us / 1000"
      />
      
      <div class="comparison-table">
        <h3>推理结果 (Top5 Logits)</h3>
        <table>
          <thead>
            <tr>
              <th>排名</th>
              <th>C++</th>
              <th>Python</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="i in 5" :key="i">
              <td>#{{ i }}</td>
              <td>{{ formatLogit(cppResult.top5[i-1]) }}</td>
              <td>{{ formatLogit(pythonResult.top5[i-1]) }}</td>
            </tr>
          </tbody>
        </table>
        <p class="note">注：由于浮点运算差异，数值可能略有不同</p>
      </div>
    </div>
  </div>
</template>

<script>
import PerformanceChart from './components/PerformanceChart.vue'
import axios from 'axios'

export default {
  name: 'App',
  components: {
    PerformanceChart
  },
  data() {
    return {
      uploadedImage: null,
      imagePreview: null,
      cppResult: null,
      pythonResult: null
    }
  },
  methods: {
    handleFileUpload(event) {
      const file = event.target.files[0]
      this.uploadedImage = file
      this.imagePreview = URL.createObjectURL(file)
    },
    
    async runBenchmark() {
      if (!this.uploadedImage) return
      
      // 读取文件为 ArrayBuffer
      const buffer = await this.uploadedImage.arrayBuffer()
      const uint8Array = new Uint8Array(buffer)
      
      try {
        // 并发请求 C++ 和 Python 服务
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
      } catch (error) {
        console.error('请求失败:', error)
        alert('推理服务请求失败，请检查后端是否启动')
      }
    },
    
    formatLogit(value) {
      return value ? value.toFixed(4) : 'N/A'
    }
  }
}
</script>

<style>
#app {
  font-family: Arial, sans-serif;
  max-width: 900px;
  margin: 0 auto;
  padding: 20px;
}

.upload-section {
  margin: 20px 0;
}

.preview img {
  max-width: 300px;
  max-height: 300px;
  border: 1px solid #ddd;
  border-radius: 4px;
}

.comparison-table {
  margin-top: 30px;
}

table {
  width: 100%;
  border-collapse: collapse;
}

th, td {
  border: 1px solid #ddd;
  padding: 8px;
  text-align: center;
}

th {
  background-color: #f2f2f2;
}

.note {
  color: #666;
  font-size: 0.9em;
}
</style>