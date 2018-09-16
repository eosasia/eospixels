const { eos, contractPublicKey } = require(`../config`)

const { EOS_CONTRACT_NAME } = process.env

async function updateAuth() {
  // inline action required
  const auth = {
    threshold: 1,
    accounts: [
      {
        permission: { actor: EOS_CONTRACT_NAME, permission: 'eosio.code' },
        weight: 1,
      },
    ],
  }

  try {
    console.log('Updating contract eosio.code permissions')
    await eos.updateauth({
      account: EOS_CONTRACT_NAME,
      permission: 'active',
      parent: 'owner',
      auth: Object.assign({}, auth, {
        keys: [{ key: contractPublicKey, weight: 1 }],
      }),
    })
    console.log(`Updated`)
  } catch (err) {
    console.error(`Cannot update contract permission`, err)
    process.exit(1)
  }
}

module.exports = {
  updateAuth,
}
