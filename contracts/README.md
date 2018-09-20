## Setup

Install project dependencies:

```
yarn
```

### Local EOS test network extra steps

#### Run nodeos in docker:

```bash
# cd to path-to-repo/contracts frist
yarn nodeos:docker

# alias cleos
alias cleos='docker exec -it eosio /opt/eosio/bin/cleos -u http://0.0.0.0:8888 --wallet-url http://0.0.0.0:8888'
```

#### Initialize wallet if not already

```sh
cleos wallet create
# Remember to save the password
cleos wallet import --private-key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
```

#### Create EOS token if not already

Create eosio.token account:

```
cleos create account eosio eosio.token EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
```

Deploy eosio.token contract:

```
cleos set contract eosio.token contracts/eosio.token -p eosio.token@active
```

Create EOS token:

```
cleos push action eosio.token create '[ "eosio", "1000000000.0000 EOS"]' -p eosio.token
```

## Compile & Deploy

> Note: For [Kylin Testnet](https://www.cryptokylin.io/) or [Jungle Testnet](http://jungle.cryptolions.io/), you need to create the contract account on the corresponding website first, and then you also need to buy some ram using faucet supply tokens.

Copy the `.env.template` to `.env` and add your private key there and edit your contract name and network node infomations.

[Kylin Testnet](https://www.cryptokylin.io/) example configuration:

```
EOS_CONTRACT_NAME=myeospixels1
CONTRACT_PRIVATE_KEY=5......
EOS_NETWORK_PROTOCOL=https
EOS_NETWORK_HOST=api-kylin.eosasia.one
EOS_NETWORK_PORT=443
EOS_NETWORK_CHAINID=5fff1dae8dc8e2fc4d5b23b2c7665c97f9e9d8edf2b6485a86ba311c25639191
```

Compile and deploy contracts:

```sh
# Local EOS test network
yarn start:docker

# yarn start
```

## Scripts

> Tip: for local eos testnet, you won't get error detail from api respond, use `docker logs --tail 10 -f eosio` to trace the chain logs.

Clear docker data:

```sh
# stop and remove container
docker stop eosio

# remove data
yarn clear:docker
```

Update contract (recompile and deploy):

```sh
# Local EOS test network
yarn update-contract:docker

# yarn update-contract
```

Create new canvas after current canvas is done:

```
yarn @end
```

Create new canvas automatically:

```
yarn @autoend
```

Withdraw using tester account:

```sh
yarn @withdraw '0.0010 EOS'
```

Inspecting the contract's tables:

```
yarn dump-tables
```
