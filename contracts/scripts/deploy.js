const fs = require(`fs`)
const path = require(`path`)
const { eos } = require(`../config`)

const { EOS_CONTRACT_NAME } = process.env

const contractDir = `./EOSPixels`
const wasm = fs.readFileSync(path.join(contractDir, `EOSPixels.wasm`))
const abi = fs.readFileSync(path.join(contractDir, `EOSPixels.abi`))

// Publish contract to the blockchain
const codePromise = eos.setcode(EOS_CONTRACT_NAME, 0, 0, wasm)
const abiPromise = eos.setabi(EOS_CONTRACT_NAME, JSON.parse(abi))

Promise.all([codePromise, abiPromise])
  .then(() => {
    console.log(`Successfully delpoyed`)
  })
  .catch(console.log)
