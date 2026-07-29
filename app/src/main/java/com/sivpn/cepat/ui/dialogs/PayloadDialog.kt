package com.sivpn.cepat.ui.dialogs

import androidx.compose.material3.MaterialTheme
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun PayloadDialog(
    initialPayload: String,
    initialTemplateType: Int = 0,
    initialUserAgentType: Int = 0,
    initialCustomUa: String = "",
    onDismiss: () -> Unit,
    onSave: (String, Int, Int, String) -> Unit
) {
    var payloadText by remember { mutableStateOf(initialPayload) }
    var templateType by remember { mutableIntStateOf(initialTemplateType) }
    var userAgentType by remember { mutableIntStateOf(initialUserAgentType) }
    var customUa by remember { mutableStateOf(initialCustomUa) }

    var expandedTemplate by remember { mutableStateOf(false) }
    val templateOptions = listOf("Custom", "Direct", "Proxy", "WebSocket")
    
    var expandedUa by remember { mutableStateOf(false) }
    val uaOptions = listOf("Custom", "Chrome", "Firefox", "Safari", "Edge", "Android")

    Dialog(onDismissRequest = onDismiss) {
        Surface(
            shape = RoundedCornerShape(16.dp),
            color = Color.White,
            modifier = Modifier.fillMaxWidth()
        ) {
            Column(modifier = Modifier.padding(20.dp)) {
                Text(
                    text = "Edit Payload / Bug Host",
                    fontSize = 18.sp,
                    fontWeight = FontWeight.Bold,
                    color = MaterialTheme.colorScheme.onSurface
                )
                Spacer(modifier = Modifier.height(12.dp))

                ExposedDropdownMenuBox(
                    expanded = expandedTemplate,
                    onExpandedChange = { expandedTemplate = it }
                ) {
                    OutlinedTextField(
                        value = templateOptions.getOrNull(templateType) ?: "Custom",
                        onValueChange = {},
                        readOnly = true,
                        label = { Text("Template Tipe") },
                        trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = expandedTemplate) },
                        modifier = Modifier.menuAnchor().fillMaxWidth()
                    )
                    ExposedDropdownMenu(
                        expanded = expandedTemplate,
                        onDismissRequest = { expandedTemplate = false }
                    ) {
                        templateOptions.forEachIndexed { index, selectionOption ->
                            DropdownMenuItem(
                                text = { Text(selectionOption) },
                                onClick = {
                                    templateType = index
                                    expandedTemplate = false
                                }
                            )
                        }
                    }
                }

                Spacer(modifier = Modifier.height(8.dp))

                ExposedDropdownMenuBox(
                    expanded = expandedUa,
                    onExpandedChange = { expandedUa = it }
                ) {
                    OutlinedTextField(
                        value = uaOptions.getOrNull(userAgentType) ?: "Custom",
                        onValueChange = {},
                        readOnly = true,
                        label = { Text("User Agent") },
                        trailingIcon = { ExposedDropdownMenuDefaults.TrailingIcon(expanded = expandedUa) },
                        modifier = Modifier.menuAnchor().fillMaxWidth()
                    )
                    ExposedDropdownMenu(
                        expanded = expandedUa,
                        onDismissRequest = { expandedUa = false }
                    ) {
                        uaOptions.forEachIndexed { index, selectionOption ->
                            DropdownMenuItem(
                                text = { Text(selectionOption) },
                                onClick = {
                                    userAgentType = index
                                    expandedUa = false
                                }
                            )
                        }
                    }
                }

                if (userAgentType == 0) {
                    Spacer(modifier = Modifier.height(8.dp))
                    OutlinedTextField(
                        value = customUa,
                        onValueChange = { customUa = it },
                        label = { Text("Custom User Agent String") },
                        modifier = Modifier.fillMaxWidth()
                    )
                }

                Spacer(modifier = Modifier.height(8.dp))

                OutlinedTextField(
                    value = payloadText,
                    onValueChange = { payloadText = it },
                    label = { Text("Payload String (Kosongi jika pakai default template)") },
                    placeholder = { Text("GET / HTTP/1.1[crlf]Host: domain.com[crlf]...") },
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(120.dp)
                )
                
                Spacer(modifier = Modifier.height(16.dp))
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.End
                ) {
                    TextButton(onClick = onDismiss) {
                        Text("Batal", color = MaterialTheme.colorScheme.onSurfaceVariant)
                    }
                    Spacer(modifier = Modifier.width(8.dp))
                    Button(
                        onClick = {
                            onSave(payloadText.trim(), templateType, userAgentType, customUa.trim())
                            onDismiss()
                        },
                        colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF3B82F6))
                    ) {
                        Text("Simpan")
                    }
                }
            }
        }
    }
}
