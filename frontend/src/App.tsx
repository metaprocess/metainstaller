import { BrowserRouter, Routes, Route } from 'react-router-dom'
import { QueryClient, QueryClientProvider } from '@tanstack/react-query'
import Layout from './components/Layout'
import Dashboard from './components/Dashboard'
import Installation from './components/docker/Installation'
import ServiceManagement from './components/docker/ServiceManagement'
import ContainerManagement from './components/containers/ContainerManagement'
import ImageManagement from './components/images/ImageManagement'
import ProjectManagement from './components/projects/ProjectManagement'
import SystemInfo from './components/system/SystemInfo'
import LogViewer from './components/logs/LogViewer'
import './App.css'

const queryClient = new QueryClient({
  defaultOptions: {
    queries: {
      staleTime: 5000,
      refetchOnWindowFocus: false,
    },
  },
})

function App() {
  return (
    <QueryClientProvider client={queryClient}>
      <BrowserRouter>
        <Routes>
          <Route path="/" element={<Layout />}>
            <Route index element={<Dashboard />} />
            <Route path="installation" element={<Installation />} />
            <Route path="service" element={<ServiceManagement />} />
            <Route path="containers" element={<ContainerManagement />} />
            <Route path="images" element={<ImageManagement />} />
            <Route path="projects" element={<ProjectManagement />} />
            <Route path="system" element={<SystemInfo />} />
            <Route path="logs" element={<LogViewer />} />
          </Route>
        </Routes>
      </BrowserRouter>
    </QueryClientProvider>
  )
}

export default App
