package com.sivpn.cepat.ui

import androidx.compose.runtime.Composable
import com.sivpn.cepat.model.MainUiState
import com.sivpn.cepat.model.DialogUiState
import com.sivpn.cepat.ui.dialogs.*
import com.sivpn.cepat.viewmodel.DialogViewModel
import com.sivpn.cepat.viewmodel.MainViewModel

@Composable
fun MainScreenDialogs(
    uiState: MainUiState,
    dialogState: DialogUiState,
    viewModel: MainViewModel,
    dialogViewModel: DialogViewModel,
    logs: List<String>
) {
    if (dialogState.showPayloadDialog) {
        PayloadDialog(
            initialPayload = uiState.payload,
            initialTemplateType = uiState.payloadTemplateType,
            initialUserAgentType = uiState.userAgentType,
            initialCustomUa = uiState.customUserAgent,
            onDismiss = { dialogViewModel.setShowPayloadDialog(false) },
            onSave = { newPayload, templateType, uaType, customUa ->
                viewModel.updatePayload(newPayload, templateType, uaType, customUa)
            }
        )
    }

    if (dialogState.showProxyDialog) {
        ProxyDialog(
            initialProxyFullInput = if (uiState.proxyHost.isNotEmpty()) "${uiState.proxyHost}:${uiState.proxyPort}" else "",
            onDismiss = { dialogViewModel.setShowProxyDialog(false) },
            onSave = { newProxy -> viewModel.updateProxyFullInput(newProxy) }
        )
    }

    if (dialogState.showSshDialog) {
        SshDialog(
            initialSshFullInput = uiState.sshFullInput,
            onDismiss = { dialogViewModel.setShowSshDialog(false) },
            onSave = { newSsh -> viewModel.updateSshFullInput(newSsh) }
        )
    }

    if (dialogState.showProfileDialog) {
        ProfileDialog(
            currentProfile = uiState.currentProfile,
            profileList = uiState.profileList,
            onDismiss = { dialogViewModel.setShowProfileDialog(false) },
            onSelectProfile = { profile -> viewModel.selectProfile(profile) },
            onAddProfileClick = {
                dialogViewModel.setShowProfileDialog(false)
                dialogViewModel.setShowAddProfileDialog(true)
            },
            onDeleteProfile = { profile -> viewModel.deleteProfile(profile) }
        )
    }

    if (dialogState.showAddProfileDialog) {
        AddProfileDialog(
            onDismiss = { dialogViewModel.setShowAddProfileDialog(false) },
            onSave = { name -> viewModel.addProfile(name) }
        )
    }

    if (dialogState.showLogDialog) {
        LogDialog(
            logs = logs,
            onDismiss = { dialogViewModel.setShowLogDialog(false) },
            onSettingsClick = {
                // Settings action if needed
            }
        )
    }

    if (dialogState.showLimitDialog) {
        TimeLimitDialog(
            initialLimitMinutes = uiState.connectionLimitMinutes,
            initialEnabled = uiState.connectionLimitEnabled,
            onDismiss = { dialogViewModel.setShowLimitDialog(false) },
            onSave = { minutes, enabled -> viewModel.updateTimeLimit(enabled, minutes) }
        )
    }

    if (dialogState.showSplitTunnelingDialog) {
        SplitTunnelDialog(
            initialEnabled = uiState.splitTunnelingEnabled,
            initialFilterMode = uiState.appsFilterMode,
            initialBypassApps = uiState.bypassApps,
            onDismiss = { dialogViewModel.setShowSplitTunnelingDialog(false) },
            onSave = { enabled, mode, apps -> viewModel.updateSplitTunneling(enabled, mode, apps) }
        )
    }

    if (dialogState.showKillSwitchDialog) {
        KillSwitchDialog(
            initialEnabled = uiState.killSwitchEnabled,
            onDismiss = { dialogViewModel.setShowKillSwitchDialog(false) },
            onSave = { enabled -> viewModel.updateKillSwitch(enabled) }
        )
    }

    if (dialogState.showPingIntervalDialog) {
        PingIntervalDialog(
            initialInterval = uiState.keepAliveInterval,
            onDismiss = { dialogViewModel.setShowPingIntervalDialog(false) },
            onSave = { interval -> viewModel.updateKeepAliveInterval(interval) }
        )
    }

    if (dialogState.showDnsDropdown) {
        DnsDialog(
            initialDns = uiState.dns,
            onDismiss = { dialogViewModel.setShowDnsDropdown(false) },
            onSave = { dns -> viewModel.updateDns(dns) }
        )
    }
}
