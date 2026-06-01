package com.cryptoscanner.ui.adapter

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.cryptoscanner.R
import com.cryptoscanner.model.WalletResult
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class WalletAdapter(
    private val onCopyAddress: (String) -> Unit
) : ListAdapter<WalletResult, WalletAdapter.ViewHolder>(DiffCallback) {

    companion object {
        private val DATE_FORMAT = SimpleDateFormat("MM/dd HH:mm:ss", Locale.getDefault())

        private object DiffCallback : DiffUtil.ItemCallback<WalletResult>() {
            override fun areItemsTheSame(old: WalletResult, new: WalletResult) = old.id == new.id
            override fun areContentsTheSame(old: WalletResult, new: WalletResult) = old == new
        }
    }

    inner class ViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        private val tvNetworkBadge: TextView = itemView.findViewById(R.id.tv_network_badge)
        private val tvAddress: TextView = itemView.findViewById(R.id.tv_address)
        private val tvBalance: TextView = itemView.findViewById(R.id.tv_balance)
        private val tvDerivationPath: TextView = itemView.findViewById(R.id.tv_derivation_path)
        private val tvTimestamp: TextView = itemView.findViewById(R.id.tv_timestamp)

        fun bind(result: WalletResult) {
            tvNetworkBadge.text = result.network
            tvNetworkBadge.setBackgroundColor(networkColor(result.network, itemView))
            tvAddress.text = truncateAddress(result.address)
            tvBalance.text = result.balanceFormatted
            tvDerivationPath.text = result.derivationPath
            tvTimestamp.text = DATE_FORMAT.format(Date(result.timestamp))

            // Long press → copy full address
            itemView.setOnLongClickListener {
                onCopyAddress(result.address)
                true
            }
        }

        private fun truncateAddress(address: String): String {
            return if (address.length > 20) {
                "${address.take(10)}…${address.takeLast(8)}"
            } else address
        }

        private fun networkColor(network: String, view: View): Int {
            val ctx = view.context
            return when (network) {
                "BTC" -> ctx.getColor(R.color.btc_orange)
                "ETH" -> ctx.getColor(R.color.eth_purple)
                "SOL" -> ctx.getColor(R.color.sol_green)
                "MATIC" -> ctx.getColor(R.color.secondary)
                "BNB" -> ctx.getColor(R.color.warning)
                "AVAX" -> ctx.getColor(R.color.error)
                else -> ctx.getColor(R.color.primary)
            }
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_wallet, parent, false)
        return ViewHolder(view)
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        holder.bind(getItem(position))
    }
}
