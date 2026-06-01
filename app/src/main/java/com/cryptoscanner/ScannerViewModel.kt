package com.cryptoscanner

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.cryptoscanner.model.LogEntry
import com.cryptoscanner.model.LogLevel
import com.cryptoscanner.model.ScanConfig
import com.cryptoscanner.model.WalletResult
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

// ---------------------------------------------------------------------------
// Scan state
// ---------------------------------------------------------------------------

sealed class ScanState {
    object Idle : ScanState()
    data class Running(
        val mnemonicsChecked: Int = 0,
        val addressesChecked: Int = 0,
        val found: Int = 0
    ) : ScanState()
    data class Paused(val reason: String) : ScanState()
    data class Error(val message: String) : ScanState()
    object Stopped : ScanState()
}

// ---------------------------------------------------------------------------
// ViewModel
// ---------------------------------------------------------------------------

class ScannerViewModel(private val repository: WalletRepository) : ViewModel() {

    companion object {
        private const val MAX_LOG_ENTRIES = 500
    }

    // --- State flows --------------------------------------------------------

    private val _scanState = MutableStateFlow<ScanState>(ScanState.Idle)
    val scanState: StateFlow<ScanState> = _scanState.asStateFlow()

    private val _logEntries = MutableStateFlow<List<LogEntry>>(emptyList())
    val logEntries: StateFlow<List<LogEntry>> = _logEntries.asStateFlow()

    val results: Flow<List<WalletResult>> = repository.getAllResults()

    // --- Internal state -----------------------------------------------------

    private var scanJob: Job? = null

    // Statistics counters (thread-safe via viewModelScope / StateFlow)
    private var mnemonicsChecked = 0
    private var addressesChecked = 0
    private var totalFound = 0

    // ---------------------------------------------------------------------------
    // Public API
    // ---------------------------------------------------------------------------

    /**
     * Starts a continuous scan loop that generates and checks mnemonics
     * until [stopScan] is called.
     *
     * If [config.existingMnemonic] is non-null, scans that single mnemonic
     * then stops.
     */
    fun startScan(config: ScanConfig) {
        if (_scanState.value is ScanState.Running) return

        mnemonicsChecked = 0
        addressesChecked = 0
        totalFound = 0
        _scanState.value = ScanState.Running()

        appendLog(LogEntry("Scan started – networks: ${config.networks.map { it.ticker }}", LogLevel.INFO))

        scanJob = viewModelScope.launch {
            try {
                if (config.existingMnemonic != null) {
                    // Single mnemonic mode
                    appendLog(LogEntry("Using provided mnemonic…", LogLevel.INFO))
                    val found = repository.scanMnemonic(
                        mnemonic = config.existingMnemonic,
                        config = config,
                        onLog = ::appendLog
                    )
                    mnemonicsChecked++
                    addressesChecked += config.networks.size * config.addressesPerMnemonic
                    totalFound += found.size
                    updateRunningState()
                    appendLog(LogEntry("Single mnemonic scan complete.", LogLevel.INFO))
                    _scanState.value = ScanState.Stopped
                } else {
                    // Continuous loop
                    while (true) {
                        val found = repository.generateAndScan(config, ::appendLog)
                        mnemonicsChecked++
                        addressesChecked += config.networks.size * config.addressesPerMnemonic
                        totalFound += found.size
                        updateRunningState()
                    }
                }
            } catch (e: kotlinx.coroutines.CancellationException) {
                // Expected when stopScan() is called
                appendLog(LogEntry("Scan stopped by user.", LogLevel.INFO))
                _scanState.value = ScanState.Stopped
            } catch (e: Exception) {
                appendLog(LogEntry("Fatal error: ${e.message}", LogLevel.ERROR))
                _scanState.value = ScanState.Error(e.message ?: "Unknown error")
            }
        }
    }

    /**
     * Cancels the running scan job.
     */
    fun stopScan() {
        scanJob?.cancel()
        scanJob = null
        if (_scanState.value is ScanState.Running) {
            _scanState.value = ScanState.Stopped
        }
    }

    /** Clears the in-memory log list. */
    fun clearLogs() {
        _logEntries.value = emptyList()
    }

    /** Deletes all wallet results from the database. */
    fun clearResults() {
        viewModelScope.launch {
            repository.clearAllResults()
            appendLog(LogEntry("All results cleared.", LogLevel.WARNING))
        }
    }

    // ---------------------------------------------------------------------------
    // Private helpers
    // ---------------------------------------------------------------------------

    private fun appendLog(entry: LogEntry) {
        _logEntries.update { current ->
            // Most recent at index 0 for top-of-list display
            val updated = listOf(entry) + current
            if (updated.size > MAX_LOG_ENTRIES) updated.take(MAX_LOG_ENTRIES) else updated
        }
    }

    private fun updateRunningState() {
        _scanState.update {
            if (it is ScanState.Running) {
                it.copy(
                    mnemonicsChecked = mnemonicsChecked,
                    addressesChecked = addressesChecked,
                    found = totalFound
                )
            } else it
        }
    }
}

// ---------------------------------------------------------------------------
// ViewModelFactory
// ---------------------------------------------------------------------------

class ScannerViewModelFactory(
    private val repository: WalletRepository
) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(ScannerViewModel::class.java)) {
            return ScannerViewModel(repository) as T
        }
        throw IllegalArgumentException("Unknown ViewModel class: ${modelClass.name}")
    }
}
