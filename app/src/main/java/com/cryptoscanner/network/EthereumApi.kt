package com.cryptoscanner.network

import android.util.Log
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody.Companion.toRequestBody
import org.json.JSONObject
import java.math.BigDecimal
import java.math.BigInteger
import java.math.RoundingMode
import java.util.concurrent.TimeUnit
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * EVM-compatible balance checker using public JSON-RPC endpoints.
 *
 * eth_getBalance returns a hex-encoded Wei value.
 * 1 ETH = 10^18 Wei.
 *
 * Supported networks (default endpoints listed first):
 *  - Ethereum : cloudflare-eth.com, ankr, publicnode, llamarpc
 *  - Polygon  : polygon-rpc.com, ankr
 *  - BSC      : bsc-dataseed.binance.org, ankr
 *  - Avalanche: api.avax.network (C-chain), ankr
 */
class EthereumApi {

    companion object {
        private const val TAG = "EthereumApi"
        private val WEI_PER_ETH = BigDecimal("1000000000000000000") // 10^18

        // Default Ethereum endpoints
        val ETHEREUM_RPCS = listOf(
            "https://cloudflare-eth.com",
            "https://rpc.ankr.com/eth",
            "https://ethereum.publicnode.com",
            "https://eth.llamarpc.com"
        )

        // Polygon endpoints
        val POLYGON_RPCS = listOf(
            "https://polygon-rpc.com",
            "https://rpc.ankr.com/polygon",
            "https://polygon.publicnode.com"
        )

        // BSC endpoints
        val BSC_RPCS = listOf(
            "https://bsc-dataseed.binance.org",
            "https://rpc.ankr.com/bsc",
            "https://bsc.publicnode.com"
        )

        // Avalanche C-chain endpoints
        val AVAX_RPCS = listOf(
            "https://api.avax.network/ext/bc/C/rpc",
            "https://rpc.ankr.com/avalanche",
            "https://avalanche.publicnode.com/ext/bc/C/rpc"
        )

        private val JSON_MEDIA_TYPE = "application/json; charset=utf-8".toMediaType()
    }

    private val client: OkHttpClient = OkHttpClient.Builder()
        .connectTimeout(15, TimeUnit.SECONDS)
        .readTimeout(20, TimeUnit.SECONDS)
        .build()

    // ---------------------------------------------------------------------------
    // Public API
    // ---------------------------------------------------------------------------

    /**
     * Returns the ETH/EVM balance as a formatted string (e.g. "0.123456 ETH").
     *
     * @param address  EVM hex address (0x…)
     * @param rpcUrls  Override RPC list; defaults to Ethereum mainnet endpoints.
     * @param ticker   Symbol used in the formatted result string.
     */
    suspend fun getBalance(
        address: String,
        rpcUrls: List<String> = ETHEREUM_RPCS,
        ticker: String = "ETH"
    ): String = withContext(Dispatchers.IO) {
        val rotator = ApiRotator(
            endpoints = rpcUrls.map { url ->
                suspend { fetchBalance(address, url) }
            },
            maxRetriesPerEndpoint = 2,
            baseDelayMs = 500
        )
        val weiHex = rotator.execute()
        val wei = BigInteger(weiHex.removePrefix("0x"), 16)
        val eth = BigDecimal(wei).divide(WEI_PER_ETH, 8, RoundingMode.HALF_UP)
        "${eth.toPlainString()} $ticker"
    }

    /**
     * Returns the raw Wei value as a hex string for a given RPC.
     * Exposed for testing / custom callers.
     */
    suspend fun getRawWeiHex(address: String, rpcUrl: String): String =
        withContext(Dispatchers.IO) { fetchBalance(address, rpcUrl) }

    // ---------------------------------------------------------------------------
    // Network-specific convenience helpers
    // ---------------------------------------------------------------------------

    suspend fun getEthereumBalance(address: String) =
        getBalance(address, ETHEREUM_RPCS, "ETH")

    suspend fun getPolygonBalance(address: String) =
        getBalance(address, POLYGON_RPCS, "MATIC")

    suspend fun getBscBalance(address: String) =
        getBalance(address, BSC_RPCS, "BNB")

    suspend fun getAvalancheBalance(address: String) =
        getBalance(address, AVAX_RPCS, "AVAX")

    // ---------------------------------------------------------------------------
    // Private helpers
    // ---------------------------------------------------------------------------

    private fun fetchBalance(address: String, rpcUrl: String): String {
        Log.d(TAG, "eth_getBalance $address @ $rpcUrl")

        val payload = """{"jsonrpc":"2.0","method":"eth_getBalance","params":["$address","latest"],"id":1}"""
        val body = payload.toRequestBody(JSON_MEDIA_TYPE)
        val request = Request.Builder()
            .url(rpcUrl)
            .post(body)
            .header("User-Agent", "CryptoScanner/1.0")
            .header("Content-Type", "application/json")
            .build()

        client.newCall(request).execute().use { response ->
            when {
                response.code == 429 ->
                    throw RateLimitException(
                        response.header("Retry-After")
                            ?.toLongOrNull()?.times(1000) ?: 2000
                    )
                response.code >= 500 ->
                    throw ApiException("Server error from $rpcUrl", response.code)
                !response.isSuccessful ->
                    throw ApiException("HTTP ${response.code} from $rpcUrl", response.code)
            }

            val responseBody = response.body?.string()
                ?: throw ApiException("Empty body from $rpcUrl")

            val json = JSONObject(responseBody)
            if (json.has("error")) {
                val err = json.getJSONObject("error")
                throw ApiException(
                    "JSON-RPC error: ${err.optString("message")}",
                    err.optInt("code", 0)
                )
            }
            return json.getString("result")
        }
    }
}
