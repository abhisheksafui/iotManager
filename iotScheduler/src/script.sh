


for file in *.c
do 
  echo "Processing $file"
  if(grep Copyright $file);then
    echo "Found Copyright"
  else
    echo "Appending Copyright"
    cat /tmp/header.txt $file > $file.new
    mv $file.new $file
  fi
done

