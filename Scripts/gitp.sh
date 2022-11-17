url=$1
url_temp=${url##*/}
dir_name=${url_temp%.*}
echo "Cloning "${dir_name} 
git clone ${url}
echo "Updating gitignore file..."
echo -e "\n.DS_Store" >> ./${dir_name}/.gitignore
echo "Committing updates..."
cd ./${dir_name}
git add ./.gitignore
git commit -m "Update .gitignore."
echo "Done!"
