package com.cryptoscanner.model

data class LogEntry(
    val id: Long = System.currentTimeMillis(),
    val message: String,
    val level: LogLevel = LogLevel.INFO,
    val timestamp: Long = System.currentTimeMillis()
)

enum class LogLevel { INFO, SUCCESS, WARNING, ERROR }
