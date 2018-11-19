pipeline {
  agent {
    node {
      label 'master-node'
    }

  }
  stages {
    stage('Configure') {
      steps {
        cmake(installation: 'cmake', arguments: '-DCMAKE_INSTALL_PREFIX=$PWD/install', workingDir: '000build.tmp')
      }
    }
  }
}