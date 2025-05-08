package com.example.smartdehumidifier

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.google.firebase.database.*

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            IoTDashboard()
        }
    }
}

@Composable
fun IoTDashboard() {
    val database = FirebaseDatabase.getInstance("https://tugasakhirembed-default-rtdb.asia-southeast1.firebasedatabase.app/")
    val humidityRef = database.getReference("humidity")
    val modeRef = database.getReference("mode")
    val temperatureRef = database.getReference("temperature")
    val waterLevelRef = database.getReference("waterLevel")

    var humidity by remember { mutableStateOf(0) }
    var mode by remember { mutableStateOf(0) }
    var temperature by remember { mutableStateOf(0) }
    var waterLevel by remember { mutableStateOf(0) }

    // Read data from Firebase in real-time
    LaunchedEffect(Unit) {

        modeRef.addValueEventListener(object : ValueEventListener {
            override fun onDataChange(snapshot: DataSnapshot) {
                mode = snapshot.getValue(Int::class.java) ?: 0
            }
            override fun onCancelled(error: DatabaseError) {}
        })

            humidityRef.addValueEventListener(object : ValueEventListener {
                override fun onDataChange(snapshot: DataSnapshot) {
                    humidity = snapshot.getValue(Int::class.java) ?: 0
                }

                override fun onCancelled(error: DatabaseError) {}
            })

            temperatureRef.addValueEventListener(object : ValueEventListener {
                override fun onDataChange(snapshot: DataSnapshot) {
                    temperature = snapshot.getValue(Int::class.java) ?: 0
                }

                override fun onCancelled(error: DatabaseError) {}
            })

            waterLevelRef.addValueEventListener(object : ValueEventListener {
                override fun onDataChange(snapshot: DataSnapshot) {
                    waterLevel = snapshot.getValue(Int::class.java) ?: 0

                }

                override fun onCancelled(error: DatabaseError) {}
            })

    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(Color.Black)
            .padding(16.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Spacer(modifier = Modifier.height(20.dp))
        Text("SMART DEHUMIDIFIER", fontSize = 24.sp, fontWeight = FontWeight.Bold, color = Color.White)
        Spacer(modifier = Modifier.height(80.dp))

        // **Humidity Circular Gauge dengan ujung melengkung**
        Box(contentAlignment = Alignment.Center, modifier = Modifier.size(200.dp)) {
            Canvas(modifier = Modifier.size(200.dp)) {
                drawCircle(
                    color = Color.DarkGray,
                    style = Stroke(width = 80f)
                )
                drawArc(
                    color = Color.Cyan,
                    startAngle = -90f,
                    sweepAngle = (humidity / 100f) * 360f,
                    useCenter = false,
                    style = Stroke(width = 80f, cap = StrokeCap.Round) // **Cap.Round untuk ujung melengkung**
                )
            }
            Text(text = "$humidity%", fontSize = 38.sp, fontWeight = FontWeight.Bold, color = Color.White)
        }
        Spacer(modifier = Modifier.height(60.dp))

        // **Temperature & Water Level Cards dengan border ganda**
        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceEvenly) {
            StatusCard("Temperature", "$temperature°C", borderColor = Color.DarkGray, outerBorderColor = Color(0xFFE3F2FD))
            StatusCard(
                "Water Level", "$waterLevel%",
                borderColor = Color.DarkGray,
                outerBorderColor = when {
                    waterLevel <= 35 -> Color.Green
                    waterLevel <= 70 -> Color.Yellow
                    else -> Color.Red
                }
            )
        }


        Spacer(modifier = Modifier.height(20.dp))

        // **Single Button for On/Off/Auto Mode**
        Button(
            onClick = {
                val newMode = (mode + 1) % 3
                modeRef.setValue(newMode) // Update mode di Firebase
            },
            modifier = Modifier.fillMaxWidth().height(80.dp),
            colors = ButtonDefaults.buttonColors(
                when (mode) {
                    1 -> Color.Green  // ON
                    2 -> Color.Blue   // AUTO
                    else -> Color.Gray // OFF
                }
            )
        ) {
            Text(
                text = when (mode) {
                    1 -> "Turn On"
                    2 -> "Auto Mode"
                    else -> "Turn Off"
                },
                fontSize = 20.sp,
                fontWeight = FontWeight.Bold,
                color = Color.White
            )
        }
    }
}

// **Reusable Status Card (Temperature & Water Level) dengan border ganda**
@Composable
fun StatusCard(label: String, value: String, borderColor: Color, outerBorderColor: Color) {
    Box(
        modifier = Modifier
            .border(width = 6.dp, color = outerBorderColor, shape = RoundedCornerShape(12.dp)) // Border luar
            .border(width = 3.dp, color = borderColor, shape = RoundedCornerShape(12.dp)) // Border dalam
    ) {
        Card(
            modifier = Modifier
                .width(150.dp)  // Ukuran tetap
                .height(100.dp)
                .padding(8.dp),
            shape = RoundedCornerShape(12.dp),
            colors = CardDefaults.cardColors(containerColor = Color.Black) // Card tetap hitam
        ) {
            Column(
                modifier = Modifier.fillMaxSize(),
                verticalArrangement = Arrangement.Center,
                horizontalAlignment = Alignment.CenterHorizontally
            ) {
                Text(text = label, fontWeight = FontWeight.Bold, color = Color.White)
                Text(text = value, fontSize = 25.sp, color = Color.White)
            }
        }
    }
}
