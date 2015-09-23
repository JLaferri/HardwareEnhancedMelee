// parse.go
package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"strconv"
	"strings"
	"time"
)

type stockSummary struct {
	TimeSeconds     float32 `json:"timeSeconds"`
	Percent         float32 `json:"percent"`
	MoveLastHitBy   uint8   `json:"moveLastHitBy"`
	LastAnimation   uint16  `json:"lastAnimation"`
	OpeningsAllowed uint16  `json:"openingsAllowed"`
	IsStockLost     bool    `json:"isStockLost"`
}

type playerSummary struct {
	StocksRemaining           uint8          `json:"stocksRemaining"`
	Apm                       float32        `json:"apm"`
	AverageDistanceFromCenter float32        `json:"averageDistanceFromCenter"`
	PercentTimeClosestCenter  float32        `json:"percentTimeClosestCenter"`
	PercentTimeAboveOthers    float32        `json:"percentTimeAboveOthers"`
	PercentTimeInShield       float32        `json:"percentTimeInShield"`
	SecondsWithoutDamage      float32        `json:"secondsWithoutDamage"`
	RollCount                 uint16         `json:"rollCount"`
	SpotDodgeCount            uint16         `json:"spotDodgeCount"`
	AirDodgeCount             uint16         `json:"airDodgeCount"`
	RecoveryAttempts          uint16         `json:"recoveryAttempts"`
	SuccessfulRecoveries      uint16         `json:"successfulRecoveries"`
	EdgeguardChances          uint16         `json:"edgeguardChances"`
	EdgeguardConversions      uint16         `json:"edgeguardConversions"`
	NumberOfOpenings          uint16         `json:"numberOfOpenings"` //this is the number of time a player started a combo string
	AverageDamagePerString    float32        `json:"averageDamagePerString"`
	AverageTimePerString      float32        `json:"averageTimePerString"`
	AverageHitsPerString      float32        `json:"averageHitsPerString"`
	MostDamageString          float32        `json:"mostDamageString"`
	MostTimeString            uint32         `json:"mostTimeString"` //longest amount of frame for a combo string
	MostHitsString            uint16         `json:"mostHitsString"` //most amount of hits in a combo string
	Stocks                    []stockSummary `json:"stocks"`
}

type matchSummary struct {
	Timestamp time.Time

	Frames       uint32          `json:"frames"`
	FramesMissed uint32          `json:"framesMissed"`
	WinCondition uint8           `json:"winCondition"`
	Players      []playerSummary `json:"players"`
}

type playerParameters struct {
	Port       uint8 `json:"port"`
	Character  uint8 `json:"character"`
	Color      uint8 `json:"color"`
	PlayerType uint8 `json:"type"`
}

type matchParameters struct {
	Timestamp time.Time

	Stage   uint16             `json:"stage"`
	Players []playerParameters `json:"players"`
}

type gameData struct {
	params  *matchParameters
	summary *matchSummary
	winner  int
}

type setResult struct {
	sectionId   int
	roundId     int
	player1Name string
	player2Name string
	winnerName  string
	startTime   time.Time
	endTime     time.Time

	winnerId int
	games    []gameData
}

var externalCharacterNames = []string{
	"Captain Falcon",
	"Donkey Kong",
	"Fox",
	"Mr. Game & Watch",
	"Kirby",
	"Bowser",
	"Link",
	"Luigi",
	"Mario",
	"Marth",
	"Mewtwo",
	"Ness",
	"Peach",
	"Pikachu",
	"Ice Climbers",
	"Jigglypuff",
	"Samus",
	"Yoshi",
	"Zelda",
	"Sheik",
	"Falco",
	"Young Link",
	"Dr. Mario",
	"Roy",
	"Pichu",
	"Ganondorf",
	"Master Hand",
	"Wireframe Male (Boy)",
	"Wireframe Female (Girl)",
	"Giga Bowser",
	"Crazy Hand",
	"Sandbag",
	"Popo",
	"User Select(Event) / None",
}
var stages = []string{
	"Dummy",
	"TEST",
	"Fountain of Dreams",
	"Pokemon Stadium",
	"Princess Peach's Castle",
	"Kongo Jungle",
	"Brinstar",
	"Corneria",
	"Yoshi's Story",
	"Onett",
	"Mute City",
	"Rainbow Cruise",
	"Jungle Japes",
	"Great Bay",
	"Hyrule Temple",
	"Brinstar Depths",
	"Yoshi's Island",
	"Green Greens",
	"Fourside",
	"Mushroom Kingdom I",
	"Mushroom Kingdom II",
	"Akaneia",
	"Venom",
	"Poke Floats",
	"Big Blue",
	"Icicle Mountain",
	"Icetop",
	"Flat Zone",
	"Dream Land N64",
	"Yoshi's Island N64",
	"Kongo Jungle N64",
	"Battlefield",
	"Final Destination",
}

const (
	logFilePath     = "C:/HardwareEnhancedMelee/HTC_Throwdown_Stream.txt"
	resultsFilePath = "C:/HardwareEnhancedMelee/HTC_SMASHGG_Sets.txt"
)

const (
	helloMsgThreshold      = 50
	parametersMsgThreshold = 150
	logTimeFormat          = "2006-01-02 15:04:05.9999999 -0700 MST"
	resultsTimeFormat      = time.RFC3339
	bo5Id                  = 6528
)

// Filter returns a new slice holding only
// the elements of s that satisfy f()
func filter(s []gameData, fn func(gameData) bool) []gameData {
	var p []gameData // == nil
	for _, v := range s {
		if fn(v) {
			p = append(p, v)
		}
	}
	return p
}

func main() {
	bytes, err := ioutil.ReadFile(logFilePath)
	if err != nil {
		fmt.Println(err)
		return
	}
	s := string(bytes)

	lines := strings.Split(s, "\r\n")

	matches := []gameData{}
	match := gameData{}
	for _, v := range lines {
		//Separate time and json
		vSplit := strings.Split(v, "|")
		if len(vSplit) != 2 {
			fmt.Printf("Line in log %s was invalid.\n", v)
			continue
		}

		//Attempt to read time of log message
		time, err := time.Parse(logTimeFormat, vSplit[0])
		if err != nil {
			fmt.Println("Failed to parse time stamp. ", err)
			continue
		}

		jsonData := vSplit[1]

		length := len(jsonData)
		switch {
		case length < helloMsgThreshold:
			//Ignore
		case length < parametersMsgThreshold:
			params := matchParameters{}
			err := json.Unmarshal([]byte(jsonData), &params)
			if err != nil {
				fmt.Println("Error unmarshalling match parameters. ", err)
				break
			}

			params.Timestamp = time
			match.params = &params
		case match.params != nil:
			summary := matchSummary{}
			err := json.Unmarshal([]byte(jsonData), &summary)
			if err != nil {
				fmt.Println("Error unmarshalling match summary. ", err)
				break
			}

			summary.Timestamp = time
			match.summary = &summary
			matches = append(matches, match)
			match = gameData{}
		}
	}

	var cpuCount, earlyRqCount, lowApmCount int
	//Filter matches that are probably not real matches
	matches = filter(matches, func(g gameData) bool {
		for _, v := range g.params.Players {
			if v.PlayerType == 1 {
				cpuCount++
				return false //If match included a CPU, filter it out
			}
		}

		playersAbove1Stock := true
		for _, v := range g.summary.Players {
			if v.StocksRemaining <= 1 {
				playersAbove1Stock = false
			}

			if v.Apm < 30 {
				lowApmCount++
				return false
			}
		}

		if playersAbove1Stock && g.summary.WinCondition == 0 {
			earlyRqCount++
			return false
		}

		return true
	})

	fmt.Println("Filtering results:")
	fmt.Println("Low APM: ", lowApmCount)
	fmt.Println("CPU: ", cpuCount)
	fmt.Println("Early RQ: ", earlyRqCount)

	//Load results data from smashgg
	bytes, err = ioutil.ReadFile(resultsFilePath)
	if err != nil {
		fmt.Println(err)
		return
	}
	s = string(bytes)

	sets := []setResult{}
	lines = strings.Split(s, "\r\n")
	for _, v := range lines {
		result := setResult{}
		vSplit := strings.Split(v, "\t")
		if len(vSplit) != 8 {
			fmt.Println("Format of line in results file is not correct. ", v)
			continue
		}

		val, err := strconv.ParseInt(vSplit[1], 10, 0)
		if err != nil {
			fmt.Println(err)
			continue
		}
		result.sectionId = int(val)

		val, err = strconv.ParseInt(vSplit[2], 10, 0)
		if err != nil {
			fmt.Println(err)
			continue
		}
		result.roundId = int(val)

		result.player1Name = vSplit[3]
		result.player2Name = vSplit[4]

		result.startTime, err = time.Parse(resultsTimeFormat, vSplit[5])
		if err != nil {
			fmt.Println(err)
			continue
		}

		result.endTime, err = time.Parse(resultsTimeFormat, vSplit[6])
		if err != nil {
			fmt.Println(err)
			continue
		}

		result.winnerName = vSplit[7]

		sets = append(sets, result)
	}

	//Iterage through sets backwards and try to match games to it
	for i := len(sets) - 1; i >= 0; i-- {
		set := &sets[i]

		winsRequired := 2
		if set.sectionId == bo5Id {
			winsRequired = 3
		}

		var p1WinCount, p2WinCount int
		for {
			if len(matches) <= 0 {
				break
			}
			match := matches[len(matches)-1]
			matches = matches[:len(matches)-1]

			if match.summary.Players[0].StocksRemaining > match.summary.Players[1].StocksRemaining {
				p1WinCount++
				match.winner = 1
			} else if match.summary.Players[1].StocksRemaining > match.summary.Players[0].StocksRemaining {
				p2WinCount++
				match.winner = 2
			}

			set.games = append(set.games, match)

			if p1WinCount >= winsRequired {
				set.winnerId = 1
				break
			} else if p2WinCount >= winsRequired {
				set.winnerId = 2
				break
			}
		}
	}

	fmt.Println("End Time\tPlayer1\tPlayer2\tSet Winner\tGame Winner\tStage\tP1 Char\tP2 Char\tP1 Stocks\tP2 Stocks")
	for _, set := range sets {
		var p1Name, p2Name string
		if set.winnerId == 1 {
			p1Name = set.winnerName
			p2Name = set.player1Name
			if p1Name == p2Name {
				p2Name = set.player2Name
			}
		} else {
			p1Name = set.player1Name
			p2Name = set.winnerName
			if p1Name == p2Name {
				p1Name = set.player2Name
			}
		}

		for i := len(set.games) - 1; i >= 0; i-- {
			game := set.games[i]
			fmt.Printf("%s\t%s\t%s\t%d\t%d\t%s\t%s\t%s\t%d\t%d\n", game.summary.Timestamp.String(), p1Name, p2Name, set.winnerId, game.winner, stages[game.params.Stage],
				externalCharacterNames[game.params.Players[0].Character], externalCharacterNames[game.params.Players[1].Character],
				game.summary.Players[0].StocksRemaining, game.summary.Players[1].StocksRemaining)
		}
	}
}
