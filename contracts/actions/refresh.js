const { eos } = require(`../config`)
const { getErrorDetail } = require(`../utils`)

const { EOS_CONTRACT_NAME, TESTER_NAME } = process.env

async function action() {
  try {
    const contract = await eos.contract(EOS_CONTRACT_NAME)
    await contract.refresh({
      authorization: TESTER_NAME,
    })

    console.log(`SUCCESS`)
  } catch (error) {
    console.log(`FAILED`)
    console.error(`${getErrorDetail(error)}`)
  }
}

action()
