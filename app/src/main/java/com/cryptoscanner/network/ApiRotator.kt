package com.cryptoscanner.network

import android.util.Log
import kotlinx.coroutines.delay

class RateLimitException(val retryAfterMs: Long = 1000) : Exception("Rate limited")
class ApiException(message: String, val httpCode: Int = 0) : Exception(message)

/**
 * Generic API rotator with exponential back-off for rate limiting and
 * immediate fail-over for 5xx server errors.
 *
 * Rotation strategy:
 *  1. Try current endpoint up to [maxRetriesPerEndpoint] times.
 *  2. On HTTP 429 / [RateLimitException] → wait with exponential back-off,
 *     then try the NEXT endpoint.
 *  3. On HTTP 5xx / [ApiException] with code ≥ 500 → immediately try next.
 *  4. After all endpoints exhausted, rotate back to index 0.
 *  5. After a full rotation with no success, throw the last exception.
 */
class ApiRotator<T>(
    private val endpoints: List<suspend () -> T>,
    private val maxRetriesPerEndpoint: Int = 3,
    private val baseDelayMs: Long = 500,
    private val maxDelayMs: Long = 30_000
) {
    companion object {
        private const val TAG = "ApiRotator"
    }

    // Per-endpoint: next allowed call time (epoch ms)
    private val rateLimitedUntil = LongArray(endpoints.size) { 0L }

    suspend fun execute(): T {
        require(endpoints.isNotEmpty()) { "No endpoints provided" }

        var lastException: Exception? = null

        // Try each endpoint once per full rotation
        for (rotation in 0 until endpoints.size) {
            val index = rotation % endpoints.size
            val endpoint = endpoints[index]

            // Honour any rate-limit cool-down for this endpoint
            val waitMs = rateLimitedUntil[index] - System.currentTimeMillis()
            if (waitMs > 0) {
                Log.d(TAG, "Endpoint $index rate-limited, waiting ${waitMs}ms")
                delay(waitMs)
            }

            var delayMs = baseDelayMs
            for (attempt in 1..maxRetriesPerEndpoint) {
                try {
                    val result = endpoint()
                    // Success – reset rate-limit state
                    rateLimitedUntil[index] = 0L
                    return result
                } catch (e: RateLimitException) {
                    Log.w(TAG, "Endpoint $index rate-limited (attempt $attempt): ${e.message}")
                    val backOff = minOf(e.retryAfterMs.coerceAtLeast(delayMs), maxDelayMs)
                    rateLimitedUntil[index] = System.currentTimeMillis() + backOff
                    lastException = e
                    if (attempt < maxRetriesPerEndpoint) {
                        delay(backOff)
                        delayMs = minOf(delayMs * 2, maxDelayMs)
                    } else {
                        // Give up on this endpoint for now, move to next
                        break
                    }
                } catch (e: ApiException) {
                    Log.w(TAG, "Endpoint $index API error (attempt $attempt) code=${e.httpCode}: ${e.message}")
                    lastException = e
                    when {
                        e.httpCode == 429 -> {
                            // Treat as rate limit
                            val backOff = minOf(delayMs, maxDelayMs)
                            rateLimitedUntil[index] = System.currentTimeMillis() + backOff
                            if (attempt < maxRetriesPerEndpoint) {
                                delay(backOff)
                                delayMs = minOf(delayMs * 2, maxDelayMs)
                            } else {
                                break
                            }
                        }
                        e.httpCode >= 500 -> {
                            // Server error – skip endpoint immediately
                            break
                        }
                        else -> {
                            // Client error (4xx) – not transient, skip endpoint
                            break
                        }
                    }
                } catch (e: Exception) {
                    Log.w(TAG, "Endpoint $index threw (attempt $attempt): ${e.message}")
                    lastException = e
                    // Network error – try with back-off
                    if (attempt < maxRetriesPerEndpoint) {
                        delay(minOf(delayMs, maxDelayMs))
                        delayMs = minOf(delayMs * 2, maxDelayMs)
                    } else {
                        break
                    }
                }
            }
        }

        throw lastException ?: ApiException("All endpoints failed")
    }
}
