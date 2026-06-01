package com.cryptoscanner.model

import androidx.room.Entity
import androidx.room.PrimaryKey

@Entity(tableName = "wallet_results")
data class WalletResult(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val network: String,             // "BTC", "ETH", "SOL", etc.
    val addressType: String,         // "P2PKH", "Bech32", "ERC20", "SOL"
    val address: String,
    val balanceRaw: String,          // raw balance string from API
    val balanceFormatted: String,    // human-readable balance
    val mnemonicEncrypted: String,   // base64 AES-GCM encrypted
    val privateKeyEncrypted: String, // base64 AES-GCM encrypted
    val publicKey: String,
    val derivationPath: String,      // e.g. m/44'/0'/0'/0/0
    val timestamp: Long = System.currentTimeMillis()
)
