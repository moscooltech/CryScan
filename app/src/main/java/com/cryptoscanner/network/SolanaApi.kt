package com.cryptoscanner.network

import android.util.Log
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody.Companion.toRequestBody
import org.json.JSONObject
import java.math.BigDecimal
import java.math.RoundingMode
import java.util.concurrent.TimeUnit
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * Solana balance checker using public JSON-RPC endpoints.
 *
 * getBalance returns lamports (Long). 1 SOL = 1_000_000_000 lamports.
 *
 * Endpoints (tried in order via ApiRotator):
 *  1. https://api.mainnet-beta.solana.com
 *  2. https://rpc.ankr.com/solana
 *  3. https://solana-api.projectserum.com
 */
class SolanaApi {

    companion object {
        private const val TAG = "SolanaApi"
        private const val LAMPORTS_PER_SOL = 1_000_000_000L

        private val ENDPOINTS = listOf(
            "https://api.mainnet-beta.solana.com",
            "https://rpc.ankr.com/solana",
            "https://solana-api.projectserum.com"
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
     * Returns the confirmed SOL balance of [address] in lamports.
     * Tries all endpoints via [ApiRotator].
     */
    suspend fun getBalance(address: String): Long = withContext(Dispatchers.IO) {
        val rotator = ApiRotator(
            endpoints = ENDPOINTS.map { url ->
                suspend { fetchBalance(address, url) }
            },
            maxRetriesPerEndpoint = 2,
            baseDelayMs = 500
        )
        rotator.execute()
    }

    /**
     * Formats lamports as a human-readable SOL string, e.g. "1.234567890 SOL".
     */
    fun formatBalance(lamports: Long): String {
        val sol = BigDecimal(lamports)
            .divide(BigDecimal(LAMPORTS_PER_SOL), 9, RoundingMode.HALF_UP)
        return "${sol.toPlainString()} SOL"
    }

    // ---------------------------------------------------------------------------
    // Private helpers
    // ---------------------------------------------------------------------------

    private fun fetchBalance(address: String, rpcUrl: String): Long {
        Log.d(TAG, "getBalance $address @ $rpcUrl")

        val payload = """{"jsonrpc":"2.0","id":1,"method":"getBalance","params":["$address"]}"""
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

            // Result structure: {"result":{"context":{...},"value":N}}
            val resultObj = json.optJSONObject("result")
                ?: throw ApiException("Unexpected response format from $rpcUrl")

            return resultObj.getLong("value")
        }
    }
}
