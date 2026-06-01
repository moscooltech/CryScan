package com.cryptoscanner.model

data class ScanConfig(
    val networks: List<CryptoNetwork>,
    val wordCount: Int = 12,
    val passphrase: String = "",
    val addressesPerMnemonic: Int = 10,
    val existingMnemonic: String? = null // null = generate new
)

enum class CryptoNetwork(val displayName: String, val coinType: Int, val ticker: String) {
    BITCOIN("Bitcoin", 0, "BTC"),
    ETHEREUM("Ethereum", 60, "ETH"),
    SOLANA("Solana", 501, "SOL"),
    POLYGON("Polygon", 60, "MATIC"),
    BSC("BNB Chain", 60, "BNB"),
    AVALANCHE("Avalanche", 60, "AVAX")
}
