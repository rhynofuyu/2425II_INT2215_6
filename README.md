<div align="center">

# 🎮 SOKONAN
### *Thư viện bỏ hoang đang chờ bạn khám phá!*

[![C++](https://img.shields.io/badge/C++-17-blue.svg?style=flat&logo=c%2B%2B)](https://isocpp.org/)
[![SDL2](https://img.shields.io/badge/SDL2-Graphics-green.svg?style=flat)](https://www.libsdl.org/)
[![Platform](https://img.shields.io/badge/Platform-Cross--Platform-lightgrey.svg?style=flat)](https://github.com)

*Dự án lập trình game đầu tiên của mình, sử dụng ngôn ngữ C++ và thư viện đồ hoạ SDL2*

**Được truyền cảm hứng bởi game Sokoban** - tựa game đẩy và xếp hộp phổ biến trên các máy điện thoại và máy tính những năm 2000

</div>

---

## 👨‍💻 Thông tin tác giả

**Vàng Đức Hoàng** | K69I-CS8 | 24021492

---

## 📖 Thông tin trò chơi

### 🏛️ Cốt truyện
> Nhân vật chính đang ở trong một thư viện bỏ hoang, mục tiêu của nhân vật chính là cần phải tìm cách hoàn thành nhiệm vụ được giao phó và thoát khỏi nơi này.

### 🎯 Luật chơi
Người chơi cần tìm cách sử dụng các phím điều hướng:
- ⬆️ **Lên** 
- ⬇️ **Xuống** 
- ⬅️ **Trái** 
- ➡️ **Phải** 

Để di chuyển nhân vật chính và đẩy các **"kệ sách"** vào đích của chúng

---

## 🏆 Đánh giá

### 📊 Ngưỡng điểm tự đánh giá
<div align="center">
  
**8.5 - 9.0** ⭐⭐⭐⭐⭐

</div>

---

## 📚 Nguồn tham khảo

### 💡 Code & Ý tưởng
- 🎥 [Tutorial Video 1](https://www.youtube.com/watch?v=bKK74HN4T9c)
- 🎥 [Tutorial Playlist](https://www.youtube.com/watch?v=gOXg1ImX5j0&list=PLYmIsLVSssdIOn5J71CVBblPlXici1_2A)

### 🎨 Assets
- 🤖 [Sora Generate](https://sora.chatgpt.com/explore)
- 🎮 [Itch.io Game Assets](https://itch.io/game-assets)
- 📖 Tài liệu code SDL2 của **Lazyfoo**
- 🎪 **Genfest 2024**

### 🎵 Audio
- 🎶 [Pixabay](https://pixabay.com)

### 🤖 AI Support
**Mức độ sử dụng AI:** Debug, gợi ý và lên kế hoạch, cho ý tưởng

---

## ✅ Checklist tính năng

<details>
<summary><strong>🎨 Graphics & Rendering</strong></summary>

- ✅ Dùng các lệnh vẽ hình
- ✅ Texture
- ✅ Background
- ✅ Animation (hoạt hình)
- ✅ Font
- ✅ Status bar

</details>

<details>
<summary><strong>🎮 Input & Interaction</strong></summary>

- ✅ Event bàn phím
- ✅ Event chuột
- ✅ Xử lý va chạm

</details>

<details>
<summary><strong>🎯 Game Logic</strong></summary>

- ✅ Score (có tính điểm)
- ✅ Lưu điểm
- ✅ Menu
- ✅ Pause / Resume

</details>

<details>
<summary><strong>🔊 Audio System</strong></summary>

- ✅ Sound
- ✅ Sound on/off
- ✅ Background music

</details>

---

## 🌟 Điểm nổi bật

### 🏗️ **Kiến trúc & Thiết kế**
- 📦 Kiến trúc module hóa với các file header riêng biệt
- 🔧 Sử dụng đầy đủ hệ sinh thái SDL2 (SDL_image, SDL_mixer, SDL_ttf)
- 🖼️ Quản lý tài nguyên đồ họa qua TextureManager

### 🧠 **Thuật toán & Logic**
- 🤖 Thuật toán solver tự động giải màn chơi
- 🎮 Hệ thống quản lý màn chơi (level system)
- 💥 Cơ chế phát hiện va chạm (cho tường, hộp, người chơi)

### 💾 **Quản lý dữ liệu**
- 🏆 Hệ thống lưu/nạp điểm cao (highscores)
- ⏪ Cơ chế hoàn tác (undo system)
- 🎛️ Quản lý trạng thái game (game state management)

### 🎨 **Giao diện & UX**
- ⌨️ Xử lý input theo trạng thái game
- 🖥️ Render menu và UI với SDL_ttf

---

## 📸 Demo

<div align="center">

### 🏠 Menu chính
![Menu](https://github.com/user-attachments/assets/cad0fa62-c399-4dc1-9064-442aa371d66e)

### 🎮 Gameplay
<table>
  <tr>
    <td align="center">
      <img src="https://github.com/user-attachments/assets/5ad888f6-86dd-4cfe-8847-8d0a23c42cc8" width="300"/>
      <br><em>Level 1</em>
    </td>
    <td align="center">
      <img src="https://github.com/user-attachments/assets/d275999f-1227-4a2a-b279-fa0298f20deb" width="300"/>
      <br><em>Level 2</em>
    </td>
  </tr>
</table>

### ⚙️ Settings & Features
<table>
  <tr>
    <td align="center">
      <img src="https://github.com/user-attachments/assets/2cea8bd9-4cf4-4fc4-ab10-27d0e64c5caa" width="200"/>
      <br><em>Settings</em>
    </td>
    <td align="center">
      <img src="https://github.com/user-attachments/assets/98e0c593-98a9-4c7b-a322-b9aa5d5f8bd0" width="200"/>
      <br><em>Level Select</em>
    </td>
    <td align="center">
      <img src="https://github.com/user-attachments/assets/dadaa4d4-2205-48fd-ad78-b4c4bca32e64" width="200"/>
      <br><em>Skin Select</em>
    </td>
  </tr>
</table>

### 🎯 Advanced Features
<table>
  <tr>
    <td align="center">
      <img src="https://github.com/user-attachments/assets/5932cde7-af5d-47a4-b21e-66701dabab52" width="250"/>
      <br><em>Solver Algorithm</em>
    </td>
    <td align="center">
      <img src="https://github.com/user-attachments/assets/c8e53ccc-05fb-4df2-914f-ca32f59739e2" width="250"/>
      <br><em>Statistics</em>
    </td>
  </tr>
</table>

### 🏆 Game Progress
<table>
  <tr>
    <td align="center">
      <img src="https://github.com/user-attachments/assets/91c112c3-8c17-41ba-a9b1-1ddf75b23e2e" width="200"/>
      <br><em>Level Complete</em>
    </td>
    <td align="center">
      <img src="https://github.com/user-attachments/assets/23fe2ba2-22dc-4049-ba44-2a57bcbd3776" width="200"/>
      <br><em>High Scores</em>
    </td>
    <td align="center">
      <img src="https://github.com/user-attachments/assets/0cdff71a-6ab5-44cd-b662-f465771fbcc6" width="200"/>
      <br><em>Tutorial</em>
    </td>
    <td align="center">
      <img src="https://github.com/user-attachments/assets/072410e5-8ee6-49cc-9c89-e44bb2ef0a0c" width="200"/>
      <br><em>Final Level</em>
    </td>
  </tr>
</table>

</div>

---

<div align="center">

**🎮 Chúc bạn chơi vui vẻ! 🎮**

*Made with ❤️ and C++*

</div>











