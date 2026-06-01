package com.cryptoscanner.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.RadioGroup
import android.widget.TextView
import androidx.core.view.isVisible
import androidx.fragment.app.Fragment
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.cryptoscanner.MainActivity
import com.cryptoscanner.R
import com.cryptoscanner.ScanState
import com.cryptoscanner.ScannerViewModel
import com.cryptoscanner.model.CryptoNetwork
import com.cryptoscanner.model.ScanConfig
import com.cryptoscanner.ui.adapter.LogAdapter
import com.google.android.material.chip.Chip
import com.google.android.material.chip.ChipGroup
import kotlinx.coroutines.launch

class ScanFragment : Fragment() {

    private lateinit var viewModel: ScannerViewModel

    // Views
    private lateinit var chipGroupNetworks: ChipGroup
    private lateinit var radioGroupWordCount: RadioGroup
    private lateinit var etPassphrase: EditText
    private lateinit var tvMnemonicToggle: TextView
    private lateinit var layoutMnemonicOverride: LinearLayout
    private lateinit var etMnemonic: EditText
    private lateinit var btnStartStop: Button
    private lateinit var btnGenerate: Button
    private lateinit var tvMnemonicsChecked: TextView
    private lateinit var tvAddressesChecked: TextView
    private lateinit var tvFound: TextView
    private lateinit var rvLogs: RecyclerView

    private lateinit var logAdapter: LogAdapter

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View = inflater.inflate(R.layout.fragment_scan, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        viewModel = (requireActivity() as MainActivity).viewModel

        bindViews(view)
        setupNetworkChips()
        setupWordCountRadio()
        setupMnemonicToggle()
        setupButtons()
        setupLogRecycler()
        observeViewModel()
    }

    // ---------------------------------------------------------------------------
    // Setup
    // ---------------------------------------------------------------------------

    private fun bindViews(view: View) {
        chipGroupNetworks = view.findViewById(R.id.chip_group_networks)
        radioGroupWordCount = view.findViewById(R.id.radio_group_word_count)
        etPassphrase = view.findViewById(R.id.et_passphrase)
        tvMnemonicToggle = view.findViewById(R.id.tv_mnemonic_toggle)
        layoutMnemonicOverride = view.findViewById(R.id.layout_mnemonic_override)
        etMnemonic = view.findViewById(R.id.et_mnemonic)
        btnStartStop = view.findViewById(R.id.btn_start_stop)
        btnGenerate = view.findViewById(R.id.btn_generate)
        tvMnemonicsChecked = view.findViewById(R.id.tv_mnemonics_checked)
        tvAddressesChecked = view.findViewById(R.id.tv_addresses_checked)
        tvFound = view.findViewById(R.id.tv_found)
        rvLogs = view.findViewById(R.id.rv_logs)
    }

    private fun setupNetworkChips() {
        CryptoNetwork.values().forEach { network ->
            val chip = Chip(requireContext()).apply {
                text = network.ticker
                isCheckable = true
                isChecked = network == CryptoNetwork.BITCOIN || network == CryptoNetwork.ETHEREUM
                chipBackgroundColor = resources.getColorStateList(R.color.chip_background_selector, null)
                setTextColor(resources.getColorStateList(R.color.chip_text_selector, null))
                tag = network
            }
            chipGroupNetworks.addView(chip)
        }
    }

    private fun setupWordCountRadio() {
        // Default selection: 12 words (first radio button)
        radioGroupWordCount.check(R.id.radio_12)
    }

    private fun setupMnemonicToggle() {
        layoutMnemonicOverride.isVisible = false
        tvMnemonicToggle.setOnClickListener {
            val isVisible = layoutMnemonicOverride.isVisible
            layoutMnemonicOverride.isVisible = !isVisible
            tvMnemonicToggle.text = if (!isVisible)
                getString(R.string.mnemonic_toggle_collapse)
            else
                getString(R.string.mnemonic_toggle_expand)
        }
    }

    private fun setupButtons() {
        btnStartStop.setOnClickListener {
            when (viewModel.scanState.value) {
                is ScanState.Running -> viewModel.stopScan()
                else -> startScan()
            }
        }

        btnGenerate.setOnClickListener {
            val wordCount = getSelectedWordCount()
            try {
                val mnemonic = com.cryptoscanner.NativeBridge.generateMnemonic(wordCount)
                etMnemonic.setText(mnemonic)
                layoutMnemonicOverride.isVisible = true
                tvMnemonicToggle.text = getString(R.string.mnemonic_toggle_collapse)
            } catch (e: Exception) {
                // Native library may not be present in debug; show placeholder
                etMnemonic.setText(getString(R.string.mnemonic_placeholder))
            }
        }
    }

    private fun setupLogRecycler() {
        logAdapter = LogAdapter()
        rvLogs.apply {
            adapter = logAdapter
            layoutManager = LinearLayoutManager(requireContext()).also {
                it.reverseLayout = false
                it.stackFromEnd = false
            }
        }
    }

    // ---------------------------------------------------------------------------
    // ViewModel observation
    // ---------------------------------------------------------------------------

    private fun observeViewModel() {
        viewLifecycleOwner.lifecycleScope.launch {
            viewLifecycleOwner.repeatOnLifecycle(Lifecycle.State.STARTED) {
                launch { viewModel.scanState.collect { onScanStateChanged(it) } }
                launch { viewModel.logEntries.collect { logAdapter.submitList(it) } }
            }
        }
    }

    private fun onScanStateChanged(state: ScanState) {
        when (state) {
            is ScanState.Running -> {
                btnStartStop.text = getString(R.string.btn_stop)
                btnStartStop.setBackgroundColor(
                    requireContext().getColor(R.color.error)
                )
                tvMnemonicsChecked.text = state.mnemonicsChecked.toString()
                tvAddressesChecked.text = state.addressesChecked.toString()
                tvFound.text = state.found.toString()
            }
            is ScanState.Idle, is ScanState.Stopped, is ScanState.Error -> {
                btnStartStop.text = getString(R.string.btn_start)
                btnStartStop.setBackgroundColor(
                    requireContext().getColor(R.color.primary)
                )
            }
            else -> {}
        }
    }

    // ---------------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------------

    private fun startScan() {
        val selectedNetworks = getSelectedNetworks()
        if (selectedNetworks.isEmpty()) {
            // Require at least one network
            return
        }
        val config = ScanConfig(
            networks = selectedNetworks,
            wordCount = getSelectedWordCount(),
            passphrase = etPassphrase.text.toString(),
            addressesPerMnemonic = 10,
            existingMnemonic = etMnemonic.text.toString().trim().ifBlank { null }
        )
        viewModel.startScan(config)
    }

    private fun getSelectedNetworks(): List<CryptoNetwork> {
        val selected = mutableListOf<CryptoNetwork>()
        for (i in 0 until chipGroupNetworks.childCount) {
            val chip = chipGroupNetworks.getChildAt(i) as? Chip ?: continue
            if (chip.isChecked) {
                selected.add(chip.tag as CryptoNetwork)
            }
        }
        return selected
    }

    private fun getSelectedWordCount(): Int = when (radioGroupWordCount.checkedRadioButtonId) {
        R.id.radio_12 -> 12
        R.id.radio_15 -> 15
        R.id.radio_18 -> 18
        R.id.radio_21 -> 21
        R.id.radio_24 -> 24
        else -> 12
    }
}
