const { eos, contractPublicKey, testerPublicKey } = require(`../config`)
const { getErrorDetail } = require(`../utils`)
const { updateAuth } = require(`./_update_auth`)

const { EOS_CONTRACT_NAME, TESTER_NAME } = process.env

async function createAccount(name, publicKey) {
  try {
    await eos.getAccount(name)
    console.log(`"${name}" already exists: ${publicKey}`)

    return
    // no error => account already exists
  } catch (e) {
    // error => account does not exist yet
  }
  console.log(`Creating "${name}" ...`)
  await eos.transaction((tr) => {
    tr.newaccount({
      creator: `eosio`,
      name,
      owner: publicKey,
      active: publicKey,
    })

    // tr.buyrambytes({
    //   payer: name,
    //   receiver: name,
    //   bytes: 8192,
    // })

    // tr.delegatebw({
    //     from: name,
    //     receiver: name,
    //     stake_net_quantity: `10.0000 EOS`,
    //     stake_cpu_quantity: `10.0000 EOS`,
    //     transfer: 0,
    // })
  })

  await eos.issue(
    {
      to: name,
      quantity: `1000.0000 EOS`,
      memo: `Happy spending`,
    },
    { authorization: 'eosio' },
  )
  console.log(`Created`)
}

async function init() {
  try {
    await createAccount(EOS_CONTRACT_NAME, contractPublicKey)
    await createAccount(TESTER_NAME, testerPublicKey)
  } catch (error) {
    console.error(
      `Cannot create account ${EOS_CONTRACT_NAME} "${getErrorDetail(error)}"`,
    )
    process.exit(1)
  }

  await updateAuth()
}

init()
