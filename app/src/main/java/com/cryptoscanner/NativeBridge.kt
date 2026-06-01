package com.cryptoscanner

object NativeBridge {
    init { System.loadLibrary("cryptoscanner") }

    external fun generateMnemonic(wordCount: Int): String
    external fun validateMnemonic(mnemonic: String): Boolean
    external fun deriveBitcoinAddress(mnemonic: String, passphrase: String, index: Int): String
    external fun deriveBitcoinBech32Address(mnemonic: String, passphrase: String, index: Int): String
    external fun deriveEthereumAddress(mnemonic: String, passphrase: String, index: Int): String
    external fun deriveSolanaAddress(mnemonic: String, passphrase: String, index: Int): String
    external fun deriveAddresses(mnemonic: String, passphrase: String, coinType: Int, count: Int): String
    external fun getPrivateKeyHex(mnemonic: String, passphrase: String, coinType: Int, index: Int): String
}
