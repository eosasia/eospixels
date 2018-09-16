const { eos } = require(`../config`)
const { getErrorDetail } = require(`../utils`)

const { EOS_CONTRACT_NAME } = process.env

async function buyRam() {
  try {
    await eos.transaction((tr) => {
      tr.buyrambytes({
        payer: EOS_CONTRACT_NAME,
        receiver: EOS_CONTRACT_NAME,
        bytes: 4096,
      })

      // tr.delegatebw({
      //     from: EOS_CONTRACT_NAME,
      //     receiver: EOS_CONTRACT_NAME,
      //     stake_net_quantity: `1.0000 EOS`,
      //     stake_cpu_quantity: `1.0000 EOS`,
      //     transfer: 0,
      // })
    })
  } catch (error) {
    console.error(`${getErrorDetail(error)}`)
  }
}

buyRam()
