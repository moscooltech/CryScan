package com.cryptoscanner

import android.content.Context
import android.util.Log
import com.cryptoscanner.model.CryptoNetwork
import com.cryptoscanner.model.LogEntry
import com.cryptoscanner.model.LogLevel
import com.cryptoscanner.model.ScanConfig
import com.cryptoscanner.model.WalletResult
import com.cryptoscanner.network.BitcoinApi
import com.cryptoscanner.network.EthereumApi
import com.cryptoscanner.network.SolanaApi
import com.cryptoscanner.storage.KeystoreManager
import com.cryptoscanner.storage.WalletDatabase
import com.cryptoscanner.storage.WalletResultDao
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.asSharedFlow
import kotlinx.coroutines.withContext
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * Central repository that coordinates:
 *  - NativeBridge (C++ mnemonic/derivation)
 *  - Network API calls (Bitcoin, Ethereum, Solana)
 *  - Encrypted storage (Room + SQLCipher + Android Keystore)
 *
 * All suspend functions run on [Dispatchers.IO].
 */
class WalletRepository(context: Context) {

    companion object {
        private const val TAG = "WalletRepository"
    }

    private val dao: WalletResultDao = WalletDatabase.getInstance(context).walletResultDao()
    private val bitcoinApi = BitcoinApi()
    private val ethereumApi = EthereumApi()
    private val solanaApi = SolanaApi()

    // ---------------------------------------------------------------------------
    // Public flows
    // ---------------------------------------------------------------------------

    /** Emits the full result list whenever the database changes. */
    fun getAllResults(): Flow<List<WalletResult>> = dao.getAllResults()

    fun getResultsByNetwork(network: String): Flow<List<WalletResult>> =
        dao.getResultsByNetwork(network)

    // ---------------------------------------------------------------------------
    // Scan operations
    // ---------------------------------------------------------------------------

    /**
     * Generates a new mnemonic and scans it.
     * Convenience wrapper around [scanMnemonic].
     */
    suspend fun generateAndScan(
        config: ScanConfig,
        onLog: (LogEntry) -> Unit
    ): List<WalletResult> = withContext(Dispatchers.IO) {
        val mnemonic = NativeBridge.generateMnemonic(config.wordCount)
        log(onLog, "Generated mnemonic (${config.wordCount} words)", LogLevel.INFO)
        scanMnemonic(mnemonic, config, onLog)
    }

    /**
     * Scans [mnemonic] across all networks defined in [config].
     *
     * For each network:
     *  1. Derives [config.addressesPerMnemonic] addresses via the C++ bridge.
     *  2. Queries the appropriate balance API for each address.
     *  3. Stores any non-zero result encrypted in Room.
     *  4. Reports progress via [onLog].
     *
     * @return List of [WalletResult] with non-zero balance.
     */
    suspend fun scanMnemonic(
        mnemonic: String,
        config: ScanConfig,
        onLog: (LogEntry) -> Unit
    ): List<WalletResult> = withContext(Dispatchers.IO) {
        val found = mutableListOf<WalletResult>()
        val passphrase = config.passphrase

        for (network in config.networks) {
            log(onLog, "Scanning ${network.displayName}…", LogLevel.INFO)
            try {
                val results = scanNetwork(mnemonic, passphrase, network, config.addressesPerMnemonic, onLog)
                found.addAll(results)
                if (results.isNotEmpty()) {
                    log(onLog, "✓ Found ${results.size} non-zero ${network.ticker} address(es)!", LogLevel.SUCCESS)
                }
            } catch (e: Exception) {
                log(onLog, "Error scanning ${network.displayName}: ${e.message}", LogLevel.ERROR)
                Log.e(TAG, "Error scanning ${network.displayName}", e)
            }
        }

        found
    }

    /**
     * Clears all stored results from the database.
     */
    suspend fun clearAllResults() = withContext(Dispatchers.IO) {
        dao.deleteAll()
    }

    // ---------------------------------------------------------------------------
    // Private helpers
    // ---------------------------------------------------------------------------

    private suspend fun scanNetwork(
        mnemonic: String,
        passphrase: String,
        network: CryptoNetwork,
        addressCount: Int,
        onLog: (LogEntry) -> Unit
    ): List<WalletResult> {
        val results = mutableListOf<WalletResult>()

        // Derive all addresses at once from C++ layer (returns JSON array of objects)
        val addressesJson = NativeBridge.deriveAddresses(mnemonic, passphrase, network.coinType, addressCount)
        val addressList = parseAddressArray(addressesJson)

        for (addrObj in addressList) {
            val address = addrObj.address
            if (address.isBlank()) continue

            val index = addrObj.index
            val derivationPath = buildDerivationPath(network, index)

            log(onLog, "[${network.ticker}] Checking $derivationPath → ${truncate(address)}", LogLevel.INFO)

            try {
                val (balanceRaw, balanceFormatted) = checkBalance(network, address)
                val isNonZero = balanceRaw != "0" && balanceRaw != "0.0" && !balanceRaw.startsWith("0.00000000")

                if (isNonZero) {
                    log(onLog, "💰 Non-zero! ${network.ticker} $address = $balanceFormatted", LogLevel.SUCCESS)

                    val encryptedMnemonic = KeystoreManager.encrypt(mnemonic)
                    val privateKeyHex = addrObj.privateKeyHex
                    val encryptedPrivKey = if (privateKeyHex.isNotBlank()) {
                        KeystoreManager.encrypt(privateKeyHex)
                    } else ""

                    val result = WalletResult(
                        network = network.ticker,
                        addressType = addrObj.addressType,
                        address = address,
                        balanceRaw = balanceRaw,
                        balanceFormatted = balanceFormatted,
                        mnemonicEncrypted = encryptedMnemonic,
                        privateKeyEncrypted = encryptedPrivKey,
                        publicKey = address, // public key = address for these networks
                        derivationPath = derivationPath
                    )
                    dao.insert(result)
                    results.add(result)
                }
            } catch (e: Exception) {
                log(onLog, "  ⚠ Balance check failed for ${truncate(address)}: ${e.message}", LogLevel.WARNING)
            }
        }

        return results
    }

    private suspend fun checkBalance(
        network: CryptoNetwork,
        address: String
    ): Pair<String, String> = when (network) {
        CryptoNetwork.BITCOIN -> {
            val satoshis = bitcoinApi.getBalance(address)
            Pair(satoshis.toString(), bitcoinApi.formatBalance(satoshis))
        }
        CryptoNetwork.ETHEREUM -> {
            val formatted = ethereumApi.getEthereumBalance(address)
            val raw = formatted.split(" ").first()
            Pair(raw, formatted)
        }
        CryptoNetwork.SOLANA -> {
            val lamports = solanaApi.getBalance(address)
            Pair(lamports.toString(), solanaApi.formatBalance(lamports))
        }
        CryptoNetwork.POLYGON -> {
            val formatted = ethereumApi.getPolygonBalance(address)
            val raw = formatted.split(" ").first()
            Pair(raw, formatted)
        }
        CryptoNetwork.BSC -> {
            val formatted = ethereumApi.getBscBalance(address)
            val raw = formatted.split(" ").first()
            Pair(raw, formatted)
        }
        CryptoNetwork.AVALANCHE -> {
            val formatted = ethereumApi.getAvalancheBalance(address)
            val raw = formatted.split(" ").first()
            Pair(raw, formatted)
        }
    }

    private fun parseAddressArray(json: String): List<AddressInfo> {
        val list = mutableListOf<AddressInfo>()
        try {
            val array = org.json.JSONArray(json)
            for (i in 0 until array.length()) {
                val obj = array.getJSONObject(i)
                list.add(AddressInfo(
                    index = obj.getInt("index"),
                    address = obj.getString("address"),
                    privateKeyHex = obj.optString("privateKeyHex", ""),
                    addressType = obj.optString("addressType", "")
                ))
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to parse address array: $json", e)
        }
        return list
    }

    private data class AddressInfo(
        val index: Int,
        val address: String,
        val privateKeyHex: String,
        val addressType: String
    )


    private fun buildDerivationPath(network: CryptoNetwork, index: Int): String {
        val purpose = when (network) {
            CryptoNetwork.BITCOIN -> "44"
            else -> "44"
        }
        return "m/$purpose'/${network.coinType}'/0'/0/$index"
    }

    private fun addressTypeFor(network: CryptoNetwork): String = when (network) {
        CryptoNetwork.BITCOIN -> "P2PKH"
        CryptoNetwork.ETHEREUM -> "ERC20"
        CryptoNetwork.SOLANA -> "SOL"
        CryptoNetwork.POLYGON -> "ERC20"
        CryptoNetwork.BSC -> "BEP20"
        CryptoNetwork.AVALANCHE -> "ERC20"
    }

    private fun truncate(address: String, chars: Int = 12): String =
        if (address.length > chars * 2) "${address.take(chars)}…${address.takeLast(6)}"
        else address

    private fun log(onLog: (LogEntry) -> Unit, message: String, level: LogLevel = LogLevel.INFO) {
        onLog(LogEntry(message = message, level = level))
    }
}
