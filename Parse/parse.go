// parse.go
package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"math"
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
	Name       string
}

type matchParameters struct {
	Timestamp time.Time

	Stage   uint16             `json:"stage"`
	Players []playerParameters `json:"players"`
}

type gameData struct {
	params  *matchParameters
	summary *matchSummary

	winner int
}

type setResult struct {
	winnerName string
	loserName  string
	winner     int
	winnerWins int
	loserWins  int
	games      []gameData
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
	filePath = "C:/HardwareEnhancedMelee/BracketMatchesAsSets.txt"
)

const (
	helloMsgThreshold      = 50
	parametersMsgThreshold = 150
	timeFormat             = "2006-01-02 15:04:05.9999999 -0700 MST"
)

func main() {
	bytes, err := ioutil.ReadFile(filePath)
	if err != nil {
		fmt.Println(err)
		return
	}
	s := string(bytes)

	lines := strings.Split(s, "\r\n")

	sets := []*setResult{}
	var set *setResult
	var game *gameData
	for _, v := range lines {
		//Separate time and json

		if len(v) < 40 {
			names := strings.Split(v, "\t")
			if len(names) != 2 {
				fmt.Println("Failed to parse names. ", err)
				continue
			}

			set = &setResult{names[0], names[1], 0, 0, 0, nil}
			sets = append(sets, set)
			continue
		}

		vSplit := strings.Split(v, "|")
		//Attempt to read time of log message
		time, err := time.Parse(timeFormat, vSplit[0])
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
				fmt.Println("Error unmarshalling game parameters. ", err)
				continue
			}

			params.Timestamp = time
			game = &gameData{&params, nil, 0}
		case game.params != nil:
			summary := matchSummary{}
			err := json.Unmarshal([]byte(jsonData), &summary)
			if err != nil {
				fmt.Println("Error unmarshalling game summary. ", err)
				break
			}

			summary.Timestamp = time
			game.summary = &summary
			set.games = append(set.games, *game)
		}
	}

	//Calculate winners and losers
	for _, set := range sets {
		var p1Wins, p2Wins int
		var p1Name, p2Name string
		for i := 0; i < len(set.games); i++ {
			game := &set.games[i]
			switch {
			case game.summary.Players[1].StocksRemaining == 0:
				game.winner = 1
				p1Wins++
			case game.summary.Players[0].StocksRemaining == 0:
				game.winner = 2
				p2Wins++
			}
		}

		if p1Wins > p2Wins {
			set.winner = 1
			set.winnerWins = p1Wins
			set.loserWins = p2Wins
			p1Name = set.winnerName
			p2Name = set.loserName

		} else {
			set.winner = 2
			set.winnerWins = p2Wins
			set.loserWins = p1Wins
			p1Name = set.loserName
			p2Name = set.winnerName
		}

		for i := 0; i < len(set.games); i++ {
			game := &set.games[i]
			game.params.Players[0].Name = p1Name
			game.params.Players[1].Name = p2Name
		}
	}

	//Print set info
	fmt.Printf("Set ID\tWinner\tLoser\tWinner Wins\tLoser Wins\tDuration (Seconds)\r\n")
	for i, set := range sets {
		startTime := set.games[0].params.Timestamp
		endTime := set.games[len(set.games)-1].summary.Timestamp
		duration := endTime.Sub(startTime)
		fmt.Printf("%d\t%s\t%s\t%d\t%d\t%v\r\n", i, set.winnerName, set.loserName, set.winnerWins, set.loserWins, duration.Seconds())
	}
	fmt.Println()

	//Print game info
	fmt.Printf("Set ID\tGame ID\tPlayer\tOpponent\tWin/Loss\tStage\tCharacter\tOpponent Character\tTime (Seconds)\tStocks Remaining\tAPM\tTime Closest Center (%%)\t" +
		"Time Above (%%)\tTime Shielding (%%)\tAir Dodge Count\tRoll Count\tSpot Dodge Count\tSuccessful Recoveries\tRecovery Attempts\t" +
		"Edgeguard Conversions\tEdgeguard Chances\tNumber of Openings\tAverage Damage/String\tAverage Hits/String\tAverage Time/String (seconds)\t" +
		"Most Damage String\tMost Hits String\tMost Time String (seconds)\tKill Count\tAverage Kill Percent\tDeath Count\tAverage Death Percent\t" +
		"Damage Done\tDamage Taken\r\n")
	for i, set := range sets {
		for j, game := range set.games {
			resultStrings := []string{"Win", "Loss"}
			if game.winner == 2 {
				resultStrings[0], resultStrings[1] = resultStrings[1], resultStrings[0]
			}

			for k := 0; k < 2; k++ {
				ok := int(math.Abs(float64(k - 1)))
				cpParams := game.params.Players[k]
				opParams := game.params.Players[ok]
				cpSummary := game.summary.Players[k]
				opSummary := game.summary.Players[ok]

				var killCount, deathCount int
				var killPercentSum, deathPercentSum, damageDone, damageTaken float32
				for _, stock := range opSummary.Stocks {
					if stock.IsStockLost {
						killCount++
						killPercentSum += stock.Percent
					}
					damageDone += stock.Percent
				}

				for _, stock := range cpSummary.Stocks {
					if stock.IsStockLost {
						deathCount++
						deathPercentSum += stock.Percent
					}
					damageTaken += stock.Percent
				}

				fmt.Printf("%d\t%d\t%s\t%s\t%s\t%s\t%s\t%s\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\t%v\r\n",
					i, j, cpParams.Name, opParams.Name, resultStrings[k], stages[game.params.Stage], externalCharacterNames[cpParams.Character],
					externalCharacterNames[opParams.Character], float32(game.summary.Frames)/60, cpSummary.StocksRemaining,
					cpSummary.Apm, cpSummary.PercentTimeClosestCenter, cpSummary.PercentTimeAboveOthers, cpSummary.PercentTimeInShield,
					cpSummary.AirDodgeCount, cpSummary.RollCount, cpSummary.SpotDodgeCount, cpSummary.SuccessfulRecoveries,
					cpSummary.RecoveryAttempts, cpSummary.EdgeguardConversions, cpSummary.EdgeguardChances, cpSummary.NumberOfOpenings,
					cpSummary.AverageDamagePerString, cpSummary.AverageHitsPerString, float32(cpSummary.AverageTimePerString)/60,
					cpSummary.MostDamageString, cpSummary.MostHitsString, float32(cpSummary.MostTimeString)/60, killCount, killPercentSum/float32(killCount),
					deathCount, deathPercentSum/float32(deathCount), damageDone, damageTaken)
			}
		}
	}
	fmt.Println()

	//Print kill info
	fmt.Printf("Set ID\tGame ID\tKill ID\tPlayer\tOpponent\tWin/Loss\tStage\tCharacter\tOpponent Character\tTime (seconds)\tPercent\tKill Move\tOpenings Required\r\n")
	for i, set := range sets {
		for j, game := range set.games {
			resultStrings := []string{"Win", "Loss"}
			if game.winner == 2 {
				resultStrings[0], resultStrings[1] = resultStrings[1], resultStrings[0]
			}

			for k := 0; k < 2; k++ {
				ok := int(math.Abs(float64(k - 1)))
				cpParams := game.params.Players[k]
				opParams := game.params.Players[ok]
				opSummary := game.summary.Players[ok]

				for l, stock := range opSummary.Stocks {
					if stock.IsStockLost {
						fmt.Printf("%d\t%d\t%d\t%s\t%s\t%s\t%s\t%s\t%s\t%v\t%v\t%v\t%v\r\n",
							i, j, l, cpParams.Name, opParams.Name, resultStrings[k], stages[game.params.Stage], externalCharacterNames[cpParams.Character],
							externalCharacterNames[opParams.Character], stock.TimeSeconds, stock.Percent, stock.MoveLastHitBy, stock.OpeningsAllowed)
					}
				}
			}
		}
	}
	fmt.Println()
}
