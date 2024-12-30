#! /bin/bash

visit()
{
    for i in "$1"/*
    do
        if [ -d "$i" ]
        then
            visit "$i"
        elif [ -f "$i" ]
        then
            base=${i:$((${#1}+1))}
            if [[ "$base" == *".c" ]]
            then
                echo "$i"
            elif [[ "$base" == *".py" ]]
            then
                echo "$i"
            elif [[ "$base" == *".java" ]]
            then
                echo "$i"
            fi
        fi
    done
}

if [ $# -ge 4 ]
then
    verbose=0
    noexecute=0
    if [ $# -ge 6 ]
    then
        if [ $5 = "-v" -a $6 = "-noexecute" ]
        then
            verbose=1
            noexecute=1
        fi
    elif [ $# -eq 5 ]
    then
        if [ $5 = "-v" ]
        then
            verbose=1
        elif [ $5 = "-noexecute" ]
        then
            noexecute=1
        fi
    fi

    rm -rf "./$2"

    mkdir -p "./$2/unzip"
    mkdir -p "./$2/C"
    mkdir -p "./$2/Python"
    mkdir -p "./$2/Java"

    touch "./$2/result.csv"
    echo "student_id,type,matched,not_matched" > "./$2/result.csv"

    if (($verbose))
    then
        echo "Found `find "./$3" -type f -name "test*.txt" | wc -l` test files"
    fi
    

    for i in ./"$1"/*
    do 
        roll=${i%.zip}
        roll=${roll: -7}
        if (($verbose))
        then
            echo "Organizing files of $roll"
        fi
        unzip -qq -d "./$2/unzip/$roll" "$i"
        
        #path=`visit "./$2/unzip/$roll"`
        path=`find "./$2/unzip/$roll" -type f \( -name "*.c" -o -name "*.py" -o -name "*.java" \)`

        if [[ "$path" == *".c" ]]
        then
            mkdir -p "./$2/C/$roll"
            cp "$path" "./$2/C/$roll/main.c"
            if (($noexecute))
            then
                echo "$roll,C" >> "./$2/result.csv"
            else
                if (($verbose))
                then
                    echo "Executing files of $roll"
                fi
                error=0
                ok=0
                gcc -o "./$2/C/$roll/main.out" "./$2/C/$roll/main.c"
                for j in ./"$3"/test*.txt
                do
                    num=${j:$((${#3}+7))}
                    num=${num%.txt}
                    "./$2/C/$roll/main.out" < "$j" > "./$2/C/$roll/out$num.txt"
                    if [ `diff "./$2/C/$roll/out$num.txt" "./$4/ans$num.txt" | wc -l` -gt 0 ]
                    then
                        error=$((error + 1))
                    else
                        ok=$((ok + 1))
                    fi
                done
                echo "$roll,C,$ok,$error" >> "./$2/result.csv"
            fi
            
        elif [[ "$path" == *".py" ]]
        then
            mkdir -p "./$2/Python/$roll"
            cp "$path" "./$2/Python/$roll/main.py"
            if (($noexecute))
            then
                echo "$roll,Python" >> "./$2/result.csv"
            else
                if (($verbose))
                then
                    echo "Executing files of $roll"
                fi
                error=0
                ok=0
                for j in ./"$3"/test*.txt
                do
                    num=${j:$((${#3}+7))}
                    num=${num%.txt}
                    python3 "./$2/Python/$roll/main.py" < "$j" > "./$2/Python/$roll/out$num.txt"
                    if [ `diff "./$2/Python/$roll/out$num.txt" "./$4/ans$num.txt" | wc -l` -gt 0 ]
                    then
                        error=$((error + 1))
                    else
                        ok=$((ok + 1))
                    fi
                done
                echo "$roll,Python,$ok,$error" >> "./$2/result.csv"
            fi
        elif [[ "$path" == *".java" ]]
        then
            mkdir -p "./$2/Java/$roll"
            cp "$path" "./$2/Java/$roll/Main.java"
            # compile
            if (($noexecute))
            then
                echo "$roll,Java" >> "./$2/result.csv"
            else 
                if (($verbose))
                then
                    echo "Executing files of $roll"
                fi
                error=0
                ok=0
                javac "./$2/Java/$roll/Main.java"
                for j in ./"$3"/test*.txt
                do
                    num=${j:$((${#3}+7))}
                    num=${num%.txt}
                    java -cp "./$2/Java/$roll" Main < "$j" > "./$2/Java/$roll/out$num.txt"
                    if [ `diff "./$2/Java/$roll/out$num.txt" "./$4/ans$num.txt" | wc -l` -gt 0 ]
                    then
                        error=$((error + 1))
                    else
                        ok=$((ok + 1))
                    fi
                done
                echo "$roll,Java,$ok,$error" >> "./$2/result.csv"
            fi 

        fi

    done

    rm -rf "./$2/unzip"

else
    echo "Usage:"
    echo "./organize.sh <submission folder> <target folder> <test folder> <answer folder> [-v] [-noexecute]"
    echo ""
    echo "-v: verbose"
    echo "-noexecute: do not execute code files"
    echo ""
fi

