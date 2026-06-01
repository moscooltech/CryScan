package com.cryptoscanner.storage

import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import android.util.Base64
import android.util.Log
import java.security.KeyStore
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey
import javax.crypto.spec.GCMParameterSpec

/**
 * Android Keystore-backed AES-256-GCM encryption manager.
 *
 * Key characteristics:
 *  - Stored in the Android Hardware Security Module (if available)
 *  - Algorithm: AES/GCM/NoPadding  (256-bit key)
 *  - No user authentication required → suitable for background scanning
 *  - IV is randomly generated per encryption and prepended to ciphertext
 *
 * Encrypted format (Base64):  [12-byte IV] || [ciphertext + 16-byte GCM tag]
 */
object KeystoreManager {

    private const val TAG = "KeystoreManager"
    private const val KEY_ALIAS = "CryptoScannerKey"
    private const val ANDROID_KEYSTORE = "AndroidKeyStore"
    private const val TRANSFORMATION = "AES/GCM/NoPadding"
    private const val GCM_IV_LENGTH = 12   // bytes
    private const val GCM_TAG_LENGTH = 128 // bits

    // ---------------------------------------------------------------------------
    // Public API
    // ---------------------------------------------------------------------------

    /**
     * Encrypts [plaintext] using AES-256-GCM.
     *
     * @return Base64-encoded string containing the IV prepended to the ciphertext.
     */
    fun encrypt(plaintext: String): String {
        val cipher = Cipher.getInstance(TRANSFORMATION)
        cipher.init(Cipher.ENCRYPT_MODE, getOrCreateKey())
        val iv = cipher.iv
        val ciphertext = cipher.doFinal(plaintext.toByteArray(Charsets.UTF_8))
        // Combine IV + ciphertext
        val combined = ByteArray(iv.size + ciphertext.size)
        System.arraycopy(iv, 0, combined, 0, iv.size)
        System.arraycopy(ciphertext, 0, combined, iv.size, ciphertext.size)
        return Base64.encodeToString(combined, Base64.NO_WRAP)
    }

    /**
     * Decrypts a Base64-encoded string produced by [encrypt].
     *
     * @return The original plaintext.
     */
    fun decrypt(encryptedBase64: String): String {
        val combined = Base64.decode(encryptedBase64, Base64.NO_WRAP)
        val iv = combined.copyOfRange(0, GCM_IV_LENGTH)
        val ciphertext = combined.copyOfRange(GCM_IV_LENGTH, combined.size)

        val cipher = Cipher.getInstance(TRANSFORMATION)
        val spec = GCMParameterSpec(GCM_TAG_LENGTH, iv)
        cipher.init(Cipher.DECRYPT_MODE, getOrCreateKey(), spec)
        val plaintext = cipher.doFinal(ciphertext)
        return String(plaintext, Charsets.UTF_8)
    }

    /**
     * Generates a random 32-byte key, encrypts it, and returns it as a
     * Base64 string. Useful for deriving database passphrases.
     */
    fun generateAndEncryptRandomKey(): String {
        val random = java.security.SecureRandom()
        val keyBytes = ByteArray(32)
        random.nextBytes(keyBytes)
        val keyHex = keyBytes.joinToString("") { "%02x".format(it) }
        return encrypt(keyHex)
    }

    // ---------------------------------------------------------------------------
    // Private helpers
    // ---------------------------------------------------------------------------

    private fun getOrCreateKey(): SecretKey {
        val keyStore = KeyStore.getInstance(ANDROID_KEYSTORE).apply { load(null) }

        // Return existing key if present
        if (keyStore.containsAlias(KEY_ALIAS)) {
            val entry = keyStore.getEntry(KEY_ALIAS, null) as? KeyStore.SecretKeyEntry
            if (entry != null) return entry.secretKey
            // Alias exists but entry is invalid → regenerate
            Log.w(TAG, "Existing key entry invalid, regenerating.")
            keyStore.deleteEntry(KEY_ALIAS)
        }

        Log.d(TAG, "Generating new AES-256-GCM key in Android Keystore")
        val keyGenerator = KeyGenerator.getInstance(
            KeyProperties.KEY_ALGORITHM_AES,
            ANDROID_KEYSTORE
        )
        val keySpec = KeyGenParameterSpec.Builder(
            KEY_ALIAS,
            KeyProperties.PURPOSE_ENCRYPT or KeyProperties.PURPOSE_DECRYPT
        )
            .setKeySize(256)
            .setBlockModes(KeyProperties.BLOCK_MODE_GCM)
            .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
            .setRandomizedEncryptionRequired(true)
            // No user authentication required → background-safe
            .setUserAuthenticationRequired(false)
            .build()

        keyGenerator.init(keySpec)
        return keyGenerator.generateKey()
    }
}
