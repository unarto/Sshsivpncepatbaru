package com.sivpn.cepat.model

data class SshConfig(
    val host: String = "",
    val port: Int = 22,
    val username: String = "",
    val password: String = "",
    val payload: String = "",
    val payloadTemplateType: Int = 0,
    val userAgentType: Int = 0,
    val customUserAgent: String = ""
)
