<script setup>

let output = defineModel("output", {type:String, default:""});
let input = defineModel("input", {type:String, default:""})

async function submitCode() {
  const res = await fetch("http://localhost:8080/python", {
    method:"POST",
    headers:{
      "Content-Type": "application/json",
    },
    body: JSON.stringify({code: input.value})
  }).then(res => res.json());
  output.value = "" + res.output;
}

</script>

<template>
  <div class="container">
    <div class="content">
      <label for="code" class="custom_label">Input code:</label>
      <textarea id="code" spellcheck="false" v-model="input"></textarea>
      <button class="compile" @click="submitCode()">Compile and run</button>
      <label for="output">Output:</label>
      <textarea readonly="readonly" id="output" v-model="output"></textarea>
    </div>
  </div>
</template>

<style scoped>
.body {
  margin: 0;
}
.container {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
}

.content {
  margin-top: 50px;
  display: flex;
  flex-direction: column;
  height: 100%;
}

#code {
  width: 800px;
  height: 400px;
  margin-top: 5px;
  margin-bottom: 5px;
  resize: none;
  font-family: 'Courier New', Courier, monospace;
  background-color: #414868;
  -webkit-text-fill-color:#c0caf5;
  border: 1px solid #414868;
  caret-color: #c0caf5;
}
#code:focus-within {
  outline:none;
  border: 1px solid #b4f9f8;
}

.compile {
  margin-bottom: 5px;
  -webkit-text-fill-color: #9ece6a;
  font-family: 'Courier New', Courier, monospace;
  font-weight: bold;
  background-color: #414868;
  width: 150px;
  height: 40px;
  border-width: 4px;
  border-style: solid;
  border-image: linear-gradient(45deg, #565f89, #bb9af7) 1;
  transition: border-image 500ms;
}
.compile:hover {
  cursor:pointer;
  border-style: solid;
  border-width: 5px;
  border-image: linear-gradient(45deg, #9ece6a, #2ac3de) 1;
  transition: border-image 500ms;
}

#output {
  margin-top: 5px;
  resize: none;
  width: 800px;
  height: 100px;
}
</style>
