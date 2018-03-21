package main

import (
	"flag"
	"fmt"
)

func main() {
	n := flag.Int("n", 0, "number")
	flag.Parse()
	if *n == 0 {
		return
	}

	var	(
		matrix [][]int
		counter int
	)

	size := 2 * *n - 1
	for r := 1; r <= size; r++ {
		var row []int
		for c := 1; c <= size; c++ {
			counter++
			row = append(row, counter)
		}
		matrix = append(matrix, row)
		fmt.Println(row)
	}
	fmt.Println()

	startRow, startColumn := size / 2, size / 2
	curRow, curColumn := 0, 0

	intervalBegin, intervalEnd := 0, 1
	iteration := 1

	result := make([]int, 0, size * size)
	result = append(result, matrix[startRow][startColumn])

	for iteration <= size / 2 {
		curColumn = startColumn + -1 * iteration
		for r := -1 * intervalBegin; r <= intervalEnd; r++ {
			curRow = startRow + r
			result = append(result, matrix[curRow][curColumn])
		}

		curRow = startRow + iteration
		for c := -1 * intervalBegin; c <= intervalEnd; c++ {
			curColumn = startColumn + c
			result = append(result, matrix[curRow][curColumn])
		}

		curColumn = startColumn + iteration
		for r := intervalBegin; r >= (-1) * intervalEnd; r-- {
			curRow = startRow + r
			result = append(result, matrix[curRow][curColumn])
		}
		curRow = startRow + -1 * iteration
		for c := intervalBegin; c >= -1 * intervalEnd; c-- {
			curColumn = startColumn + c
			result = append(result, matrix[curRow][curColumn])
		}

		intervalBegin++
		intervalEnd++
		iteration++
	}

	fmt.Println(result)
}