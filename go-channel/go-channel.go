package main

import (
	"fmt"
	"sync"
	"time"
)

func Benchmark(testDuration time.Duration) (int64, float64, float64) {
	// Create a buffer channel
	ch := make(chan int)
	defer close(ch)
	var wg sync.WaitGroup
	wg.Add(2)

	var count int64 = 0
	var totalLatencyPushData, totalLatencyFetchData time.Duration = 0, 0
	end := time.Now().Add(testDuration)

	// Receiver()
	go func() {
		defer wg.Done()

		for {
			start := time.Now()
			a := <-ch
			elapsed := time.Since(start)
			totalLatencyFetchData += elapsed
			if a == 0 {
				break
			}
		}
	}()

	// Sender
	go func() {
		defer wg.Done()

		for time.Now().Before(end) {
			start := time.Now()
			ch <- 1
			elapsed := time.Since(start)
			totalLatencyPushData += elapsed
			count++
		}
		start := time.Now()
		ch <- 0
		elapsed := time.Since(start)
		totalLatencyPushData += elapsed
		count++
	}()

	wg.Wait()

	avgPushLatency := float64(totalLatencyPushData.Microseconds()) / float64(count)
	avgFetchLatency := float64(totalLatencyFetchData.Microseconds()) / float64(count)

	return count, avgPushLatency, avgFetchLatency
}

func main() {
	const runs = 10
	const testDuration = 1 * time.Second

	var totalMessages int64
	var totalPushLatency float64
	var totalFetchLatency float64

	for i := 1; i <= runs; i++ {
		fmt.Printf("Run #%d...\n", i)
		count, pushLat, fetchLat := Benchmark(testDuration)
		fmt.Printf("  Messages: %d, Send Latency: %.3f µs, Receive Latency: %.3f µs\n", count, pushLat, fetchLat)

		totalMessages += count
		totalPushLatency += pushLat
		totalFetchLatency += fetchLat
	}

	fmt.Println("\n===== Average Results over 10 runs =====")
	fmt.Printf("Average Messages: %d\n", totalMessages/runs)
	fmt.Printf("Average Send Latency: %.3f µs\n", totalPushLatency/float64(runs))
	fmt.Printf("Average Receive Latency: %.3f µs\n", totalFetchLatency/float64(runs))
}
