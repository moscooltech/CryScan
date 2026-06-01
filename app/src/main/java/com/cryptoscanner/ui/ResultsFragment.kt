package com.cryptoscanner.ui

import android.content.ClipData
import android.content.ClipboardManager
import android.content.Context
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.core.view.isVisible
import androidx.fragment.app.Fragment
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.cryptoscanner.MainActivity
import com.cryptoscanner.R
import com.cryptoscanner.ScannerViewModel
import com.cryptoscanner.model.WalletResult
import com.cryptoscanner.ui.adapter.WalletAdapter
import kotlinx.coroutines.launch
import org.json.JSONArray
import org.json.JSONObject

class ResultsFragment : Fragment() {

    private lateinit var viewModel: ScannerViewModel

    private lateinit var rvResults: RecyclerView
    private lateinit var tvEmptyState: TextView
    private lateinit var btnClearAll: Button
    private lateinit var btnExport: Button

    private lateinit var walletAdapter: WalletAdapter

    // Hold current results for export
    private var currentResults: List<WalletResult> = emptyList()

    override fun onCreateView(
        inflater: LayoutInflater,
        container: ViewGroup?,
        savedInstanceState: Bundle?
    ): View = inflater.inflate(R.layout.fragment_results, container, false)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        viewModel = (requireActivity() as MainActivity).viewModel

        bindViews(view)
        setupRecycler()
        setupButtons()
        observeViewModel()
    }

    // ---------------------------------------------------------------------------
    // Setup
    // ---------------------------------------------------------------------------

    private fun bindViews(view: View) {
        rvResults = view.findViewById(R.id.rv_results)
        tvEmptyState = view.findViewById(R.id.tv_empty_state)
        btnClearAll = view.findViewById(R.id.btn_clear_all)
        btnExport = view.findViewById(R.id.btn_export)
    }

    private fun setupRecycler() {
        walletAdapter = WalletAdapter(
            onCopyAddress = { address -> copyToClipboard("Address", address) }
        )
        rvResults.apply {
            adapter = walletAdapter
            layoutManager = LinearLayoutManager(requireContext())
        }
    }

    private fun setupButtons() {
        btnClearAll.setOnClickListener {
            AlertDialog.Builder(requireContext())
                .setTitle(getString(R.string.dialog_clear_title))
                .setMessage(getString(R.string.dialog_clear_message))
                .setPositiveButton(getString(R.string.dialog_clear_confirm)) { _, _ ->
                    viewModel.clearResults()
                    Toast.makeText(requireContext(), getString(R.string.results_cleared), Toast.LENGTH_SHORT).show()
                }
                .setNegativeButton(getString(R.string.cancel), null)
                .show()
        }

        btnExport.setOnClickListener {
            exportResultsToClipboard()
        }
    }

    // ---------------------------------------------------------------------------
    // ViewModel observation
    // ---------------------------------------------------------------------------

    private fun observeViewModel() {
        viewLifecycleOwner.lifecycleScope.launch {
            viewLifecycleOwner.repeatOnLifecycle(Lifecycle.State.STARTED) {
                viewModel.results.collect { results ->
                    currentResults = results
                    walletAdapter.submitList(results)
                    updateEmptyState(results.isEmpty())
                }
            }
        }
    }

    // ---------------------------------------------------------------------------
    // Helpers
    // ---------------------------------------------------------------------------

    private fun updateEmptyState(isEmpty: Boolean) {
        tvEmptyState.isVisible = isEmpty
        rvResults.isVisible = !isEmpty
        btnExport.isEnabled = !isEmpty
        btnClearAll.isEnabled = !isEmpty
    }

    private fun copyToClipboard(label: String, text: String) {
        val clipboard = requireContext().getSystemService(Context.CLIPBOARD_SERVICE) as ClipboardManager
        clipboard.setPrimaryClip(ClipData.newPlainText(label, text))
        Toast.makeText(requireContext(), getString(R.string.copied_to_clipboard), Toast.LENGTH_SHORT).show()
    }

    private fun exportResultsToClipboard() {
        if (currentResults.isEmpty()) return

        val jsonArray = JSONArray()
        currentResults.forEach { result ->
            val obj = JSONObject().apply {
                put("network", result.network)
                put("addressType", result.addressType)
                put("address", result.address)
                put("balance", result.balanceFormatted)
                put("derivationPath", result.derivationPath)
                put("timestamp", result.timestamp)
            }
            jsonArray.put(obj)
        }

        val json = jsonArray.toString(2)
        copyToClipboard("CryptoScanner Results", json)
        Toast.makeText(
            requireContext(),
            getString(R.string.exported_results, currentResults.size),
            Toast.LENGTH_LONG
        ).show()
    }
}
