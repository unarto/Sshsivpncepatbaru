package com.sivpn.cepat.viewmodel

import com.sivpn.cepat.config.DefaultValues
import com.sivpn.cepat.model.MainUiState
import com.sivpn.cepat.model.SshConfig
import com.sivpn.cepat.parser.SshParser
import com.sivpn.cepat.config.SettingsManager
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.update

class MainSshController(
    private val settingsManager: SettingsManager,
    private val _uiState: MutableStateFlow<MainUiState>
) {
    private companion object {
        const val DEFAULT_SSH_PORT = DefaultValues.DEFAULT_SSH_PORT
    }

    private fun updateState(transform: (MainUiState) -> MainUiState) {
        _uiState.update(transform)
    }

    fun loadSettings() {
        val config = settingsManager.getSshConfig()
        val portStr = config.port.toString()
        updateState { state ->
            state.copy(
                sshHost = config.host,
                sshPort = portStr,
                sshUsername = config.username,
                sshPassword = config.password,
                sshFullInput = "\${config.host}:\$portStr@\${config.username}:\${config.password}",
                payload = config.payload,
                payloadTemplateType = config.payloadTemplateType,
                userAgentType = config.userAgentType,
                customUserAgent = config.customUserAgent
            )
        }
    }

    fun updateSshFullInput(input: String) {
        val creds = SshParser.parseFullInput(input, _uiState.value.sshPort)
        updateState {
            it.copy(
                sshFullInput = input,
                sshHost = creds.host,
                sshPort = creds.port,
                sshUsername = creds.username,
                sshPassword = creds.password
            )
        }
        val portInt = creds.port.toIntOrNull() ?: DEFAULT_SSH_PORT
        val config = SshConfig(
            host = creds.host,
            port = portInt,
            username = creds.username,
            password = creds.password,
            payload = _uiState.value.payload,
            payloadTemplateType = _uiState.value.payloadTemplateType,
            userAgentType = _uiState.value.userAgentType,
            customUserAgent = _uiState.value.customUserAgent
        )
        settingsManager.saveSshConfig(config)
    }

    fun updatePayload(payload: String, templateType: Int, uaType: Int, customUa: String) {
        updateState { it.copy(payload = payload, payloadTemplateType = templateType, userAgentType = uaType, customUserAgent = customUa) }
        val newConfig = SshConfig(
            host = _uiState.value.sshHost,
            port = _uiState.value.sshPort.toIntOrNull() ?: DEFAULT_SSH_PORT,
            username = _uiState.value.sshUsername,
            password = _uiState.value.sshPassword,
            payload = payload,
            payloadTemplateType = templateType,
            userAgentType = uaType,
            customUserAgent = customUa
        )
        settingsManager.saveSshConfig(newConfig)
    }
}
