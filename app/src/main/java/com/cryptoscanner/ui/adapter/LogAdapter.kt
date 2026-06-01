package com.cryptoscanner.ui.adapter

import android.graphics.Color
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.cryptoscanner.R
import com.cryptoscanner.model.LogEntry
import com.cryptoscanner.model.LogLevel
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class LogAdapter : ListAdapter<LogEntry, LogAdapter.ViewHolder>(DiffCallback) {

    companion object {
        private val TIME_FORMAT = SimpleDateFormat("HH:mm:ss.SSS", Locale.getDefault())

        private object DiffCallback : DiffUtil.ItemCallback<LogEntry>() {
            override fun areItemsTheSame(old: LogEntry, new: LogEntry) = old.id == new.id
            override fun areContentsTheSame(old: LogEntry, new: LogEntry) = old == new
        }
    }

    inner class ViewHolder(itemView: View) : RecyclerView.ViewHolder(itemView) {
        private val tvTimestamp: TextView = itemView.findViewById(R.id.tv_log_timestamp)
        private val viewLevelDot: View = itemView.findViewById(R.id.view_log_level_dot)
        private val tvMessage: TextView = itemView.findViewById(R.id.tv_log_message)

        fun bind(entry: LogEntry) {
            tvTimestamp.text = TIME_FORMAT.format(Date(entry.timestamp))
            tvMessage.text = entry.message

            val color = levelColor(entry.level)
            viewLevelDot.setBackgroundColor(color)
            tvMessage.setTextColor(color)
        }

        private fun levelColor(level: LogLevel): Int = when (level) {
            LogLevel.INFO    -> Color.parseColor("#E2E8F0") // white-ish
            LogLevel.SUCCESS -> Color.parseColor("#10B981") // green
            LogLevel.WARNING -> Color.parseColor("#F59E0B") // amber
            LogLevel.ERROR   -> Color.parseColor("#EF4444") // red
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_log, parent, false)
        return ViewHolder(view)
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        holder.bind(getItem(position))
    }
}
