package com.jinyuni.dengbei_care.telemetry

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

/** AA-005 G-5 / D-25：观测有效性（占位 vs 合法 0.0/0.0）自检 */
class BleObservationTest {

    @Test
    fun placeholder_requiresAllZeroDeviceBlock() {
        assertTrue(BleObservation.isPlaceholder("00000000", 0, 0))
        assertTrue(BleObservation.isPlaceholder("00:00:00:00", 0, 0))
        assertTrue(BleObservation.classify("00000000", 0, 0) is BleObservation.Observation.Placeholder)
    }

    @Test
    fun zeroZeroWithValidId_isValidReading() {
        val observation = BleObservation.classify("0C118224", 0, 0)
        val reading = (observation as BleObservation.Observation.Valid).reading
        assertEquals("0C118224", reading.devId)
        assertEquals(0.0, reading.temperatureC, 0.0)
        assertEquals(0.0, reading.humidityPct, 0.0)
    }

    @Test
    fun zeroIdWithNonZeroValues_isNotPlaceholder() {
        assertTrue(BleObservation.classify("00000000", 1, 0) is BleObservation.Observation.Valid)
        assertTrue(BleObservation.classify("00000000", 0, 1) is BleObservation.Observation.Valid)
        assertTrue(BleObservation.classify("00000000", -1, 0) is BleObservation.Observation.Valid)
    }

    @Test
    fun reading_normalizesIdAndKeepsNegativeTemperature() {
        val reading = (BleObservation.classify("0c:11:82:24", -52, 415) as BleObservation.Observation.Valid).reading
        assertEquals("0C118224", reading.devId)
        assertEquals(-5.2, reading.temperatureC, 0.0001)
        assertEquals(41.5, reading.humidityPct, 0.0001)
    }
}
