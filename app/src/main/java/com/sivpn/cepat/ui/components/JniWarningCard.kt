package com.sivpn.cepat.ui.components

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Security
import androidx.compose.material3.*
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp

@Composable
fun JniWarningCard(
    isNativeSshLoadedState: Boolean,
    isHevLoadedState: Boolean
) {
    if (!isNativeSshLoadedState || !isHevLoadedState) {
        Card(
            colors = CardDefaults.cardColors(
                containerColor = Color(0xFFFEE2E2),
                contentColor = Color(0xFF991B1B)
            ),
            shape = RoundedCornerShape(12.dp),
            modifier = Modifier
                .fillMaxWidth()
                .padding(bottom = 12.dp)
        ) {
            Row(
                modifier = Modifier.padding(16.dp),
                verticalAlignment = Alignment.CenterVertically
            ) {
                Icon(
                    imageVector = Icons.Default.Security,
                    contentDescription = "JNI Missed Warning",
                    tint = Color(0xFFEF4444)
                )
                Spacer(modifier = Modifier.width(12.dp))
                Column(modifier = Modifier.weight(1f)) {
                    Text(
                        text = "LIBRARY JNI NATIVE TIDAK DITEMUKAN",
                        fontWeight = FontWeight.Bold,
                        fontSize = 13.sp,
                        color = Color(0xFF991B1B)
                    )
                    Spacer(modifier = Modifier.height(4.dp))
                    val missingList = mutableListOf<String>()
                    if (!isNativeSshLoadedState) missingList.add("libssh.so")
                    if (!isHevLoadedState) missingList.add("libhev-socks5-tunnel.so")
                    Text(
                        text = "Library native berikut gagal dimuat: ${missingList.joinToString(", ")}. Pastikan berkas .so terpasang di APK agar koneksi VPN & transparan tunnel dapat berjalan stabil.",
                        fontSize = 11.sp,
                        lineHeight = 15.sp,
                        color = Color(0xFF7F1D1D)
                    )
                }
            }
        }
    }
}
